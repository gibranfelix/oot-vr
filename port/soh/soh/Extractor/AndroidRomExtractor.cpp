// SOH [Quest] JNI side of org.oot.vr.RomExtractor: checks the ROM that the player picked in the
// 2D setup panel and extracts oot.o2r / oot-mq.o2r from it, before VR starts.
//
// Java calls these three functions from its own threads while no game runs:
//   nativeCheckRom(romPath)                  -> "ok <zapdVersion> <mq>" or "error <code>"
//   nativeExtract(romPath, workDir, exportDir) -> true if exportDir holds the archive after the run
//   nativeProgress()                         -> {done, total} of the running extraction
//
// workDir must contain assets/ (Config_<ver>.xml, filelists/, symbols/, TexturePool.xml and
// xml/<ver>/). Java sets TMPDIR to its cache dir: Extractor::Mkdtemp() makes its work dir there.
#ifdef __ANDROID__

#include "Extract.h"

#include <android/log.h>
#include <jni.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

namespace {

constexpr const char* kLogTag = "OoTVR";

std::atomic<size_t> sExtractDone{ 0 };
std::atomic<size_t> sExtractTotal{ 0 };
std::atomic<bool> sExtractRunning{ false };

std::string JStringToStd(JNIEnv* env, jstring str) {
    if (str == nullptr) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(str, nullptr);
    if (chars == nullptr) {
        return {};
    }
    std::string result(chars);
    env->ReleaseStringUTFChars(str, chars);
    return result;
}

const char* RomCheckCode(Extractor::RomCheck check) {
    switch (check) {
        case Extractor::RomCheck::Ok:
            return "ok";
        case Extractor::RomCheck::Read:
            return "read";
        case Extractor::RomCheck::Compressed:
            return "compressed";
        case Extractor::RomCheck::Size:
            return "size";
        case Extractor::RomCheck::Unsupported:
        default:
            return "unsupported";
    }
}

// CallZapd() changes the process-wide cwd and only changes it back on success. This puts it back
// on every path out of nativeExtract.
class CwdRestorer {
  public:
    CwdRestorer() {
        std::error_code ec;
        mCwd = std::filesystem::current_path(ec);
        mValid = !ec;
    }
    ~CwdRestorer() {
        if (mValid) {
            std::error_code ec;
            std::filesystem::current_path(mCwd, ec);
        }
    }
    CwdRestorer(const CwdRestorer&) = delete;
    CwdRestorer& operator=(const CwdRestorer&) = delete;

  private:
    std::filesystem::path mCwd;
    bool mValid = false;
};

// CallZapd() leaves its "extractor-XXXXXX" work dir in TMPDIR when ZAPD throws. A leftover dir
// with the same name also makes the next create_symlink() fail. remove_all() does not follow the
// "assets" symlink inside it, so workDir stays intact.
void RemoveExtractorTempDirs() {
    std::error_code ec;
    const std::filesystem::path tempDir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        return;
    }
    std::filesystem::directory_iterator it(tempDir, ec);
    if (ec) {
        return;
    }
    for (const auto& entry : it) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("extractor-", 0) == 0) {
            std::error_code removeEc;
            std::filesystem::remove_all(entry.path(), removeEc);
        }
    }
}

bool ArchiveExists(const std::filesystem::path& archive) {
    std::error_code ec;
    return std::filesystem::is_regular_file(archive, ec) && std::filesystem::file_size(archive, ec) > 0 && !ec;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL Java_org_oot_vr_RomExtractor_nativeCheckRom(JNIEnv* env, jclass,
                                                                                jstring romPath) {
    std::string result;
    try {
        const std::string path = JStringToStd(env, romPath);
        auto extractor = std::make_unique<Extractor>();
        const Extractor::RomCheck check = extractor->CheckRomFile(path);
        if (check == Extractor::RomCheck::Ok) {
            result = std::string("ok ") + extractor->GetCheckedZapdVerStr() + " " +
                     (extractor->IsMasterQuest() ? "1" : "0");
        } else {
            result = std::string("error ") + RomCheckCode(check);
        }
    } catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeCheckRom: %s", e.what());
        result = "error read";
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeCheckRom: unknown exception");
        result = "error read";
    }
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "nativeCheckRom: %s", result.c_str());
    return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_oot_vr_RomExtractor_nativeExtract(JNIEnv* env, jclass,
                                                                                jstring romPath, jstring workDir,
                                                                                jstring exportDir) {
    bool expected = false;
    if (!sExtractRunning.compare_exchange_strong(expected, true)) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeExtract: an extraction is already running");
        return JNI_FALSE;
    }

    sExtractDone = 0;
    sExtractTotal = 0;
    bool ok = false;
    try {
        const std::string rom = JStringToStd(env, romPath);
        const std::string work = JStringToStd(env, workDir);
        const std::string exportPath = JStringToStd(env, exportDir);

        auto extractor = std::make_unique<Extractor>();
        const Extractor::RomCheck check = extractor->CheckRomFile(rom);
        if (check != Extractor::RomCheck::Ok) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeExtract: ROM check failed: %s",
                                RomCheckCode(check));
        } else {
            const char* archiveName = extractor->IsMasterQuest() ? "oot-mq.o2r" : "oot.o2r";
            const std::filesystem::path archive = std::filesystem::path(exportPath) / archiveName;
            // CallZapd() copies the archive into the folder it gets. It gets a staging folder next
            // to exportDir, and only a complete archive moves into exportDir, with a rename on the
            // same file system. If the process dies during the copy, exportDir holds no archive,
            // and the next start runs the setup again instead of loading a partial file.
            const std::filesystem::path staging = std::filesystem::path(exportPath) / ".extract-staging";
            // A file from an earlier run must not count as a result of this run.
            std::error_code ec;
            std::filesystem::remove(archive, ec);
            std::filesystem::remove_all(staging, ec);
            std::filesystem::create_directories(staging);

            RemoveExtractorTempDirs();
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "nativeExtract: start, version %s",
                                extractor->GetCheckedZapdVerStr());
            std::exception_ptr zapdError;
            {
                CwdRestorer cwdRestorer;
                try {
                    extractor->CallZapd(work, staging.string(), &sExtractDone, &sExtractTotal);
                } catch (...) {
                    zapdError = std::current_exception();
                }
            }
            // The cwd is back here, so the work dir is no longer in use.
            RemoveExtractorTempDirs();
            if (zapdError) {
                std::rethrow_exception(zapdError);
            }

            if (ArchiveExists(staging / archiveName)) {
                std::filesystem::rename(staging / archiveName, archive, ec);
                if (ec) {
                    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeExtract: rename failed: %s",
                                        ec.message().c_str());
                }
            }
            std::filesystem::remove_all(staging, ec);

            ok = ArchiveExists(archive);
            if (ok) {
                sExtractDone = sExtractTotal.load();
            }
            __android_log_print(ok ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kLogTag, "nativeExtract: %s %s",
                                ok ? "wrote" : "did not write", archive.c_str());
        }
    } catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeExtract: %s", e.what());
        ok = false;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "nativeExtract: unknown exception");
        ok = false;
    }

    sExtractRunning = false;
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_oot_vr_RomExtractor_nativeProgress(JNIEnv* env, jclass) {
    jlongArray result = env->NewLongArray(2);
    if (result == nullptr) {
        return nullptr; // OutOfMemoryError is pending in Java.
    }
    const jlong values[2] = { static_cast<jlong>(sExtractDone.load()), static_cast<jlong>(sExtractTotal.load()) };
    env->SetLongArrayRegion(result, 0, 2, values);
    return result;
}

#endif // __ANDROID__
