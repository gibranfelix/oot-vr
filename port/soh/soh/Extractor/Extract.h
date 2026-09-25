#ifndef EXTRACT_H
#define EXTRACT_H

#include <atomic>
#include <stdint.h>
#include <string>
#include <memory>
#include <vector>

// Values come from windows.h
#ifndef IDYES
#define IDYES 6
#endif
#ifndef IDNO
#define IDNO 7
#endif

static constexpr size_t MB_BASE = 1024 * 1024;
static constexpr size_t MB32 = 32 * MB_BASE;
static constexpr size_t MB54 = 54 * MB_BASE;
static constexpr size_t MB64 = 64 * MB_BASE;

enum class RomSearchMode {
    Both = 0,
    Vanilla = 1,
    MQ = 2,
};

class Extractor {
    std::unique_ptr<unsigned char[]> mRomData = std::make_unique<unsigned char[]>(MB64);
    std::string mCurrentRomPath;
    std::string mSearchPath;
    size_t mCurRomSize = 0;

    bool GetRomPathFromBox();

    uint32_t GetRomVerCrc() const;
    size_t GetCurRomSize() const;
    bool ValidateAndFixRom();
    bool ValidateRomSize() const;

    bool ValidateRom(bool skipCrcBox = false);
    bool ValidateNotCompressed() const;
    const char* GetZapdVerStr() const;

    void SetRomInfo(const std::string& path);

    void FilterRoms(std::vector<std::string>& roms, RomSearchMode searchMode);
    void ShowSizeErrorBox() const;
    void ShowCrcErrorBox() const;
    void ShowCompressedErrorBox() const;
    int ShowRomPickBox(uint32_t verCrc) const;
    bool ManuallySearchForRom();

  public:
    // TODO create some kind of abstraction for message boxes.
    static int ShowYesNoBox(const char* title, const char* text);
    static void ShowErrorBox(const char* title, const char* text);
    bool IsMasterQuest() const;
    bool ManuallySearchForRomMatchingType(RomSearchMode searchMode);

    void SetSearchPath(const std::string& path);
    void GetRoms(std::vector<std::string>& roms);
    bool RunFileStandalone(std::string file);
    bool Run(std::string searchPath, RomSearchMode searchMode = RomSearchMode::Both);
    bool CallZapd(std::string installPath, std::string exportdir, std::atomic<size_t>* extractCount,
                  std::atomic<size_t>* totalExtract);
    const char* GetZapdStr();
    std::string Mkdtemp();

    // SOH [Quest] The headset picks the ROM in a 2D Android panel, so the check must not open
    // SDL message boxes or file dialogs, and must not reach UNREACHABLE for an unknown ROM.
    enum class RomCheck {
        Ok,
        Read,
        Compressed,
        Size,
        Unsupported,
    };
    // SOH [Quest] Loads the ROM at path (any file name) and checks it with no UI: compressed,
    // then size, then CRC, then a known version. On Ok, CallZapd(), IsMasterQuest() and
    // GetCheckedZapdVerStr() are safe to call.
    RomCheck CheckRomFile(const std::string& path);
    // SOH [Quest] Only valid after CheckRomFile() returned RomCheck::Ok.
    const char* GetCheckedZapdVerStr() const;
};
#endif
