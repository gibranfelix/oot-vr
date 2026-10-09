extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
}

#include "VrBelt.h"
#include "VrCombat.h"

#include <libultraship/bridge/consolevariablebridge.h>
#include <vr_interface.h>

namespace {

constexpr uint32_t kBlockedFlags =
    PLAYER_STATE1_LOADING | PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD |
    PLAYER_STATE1_START_CHANGING_HELD_ITEM | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_CARRYING_ACTOR |
    PLAYER_STATE1_HANGING_OFF_LEDGE | PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_FIRST_PERSON |
    PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_DAMAGED | PLAYER_STATE1_IN_WATER |
    PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE;

VrBeltThrow::HandInput Hand(int hand) {
    float pos[3];
    float quat[4];
    VrBeltThrow::HandInput h;
    h.gripHeld = (VR_GetControllerButton(hand) & VR_BTN_GRIP) != 0;
    // An untracked hand is far from the belt.
    h.pos = VR_GetHandPose(hand, pos, quat) ? VrBeltThrow::Vec3{ pos[0], pos[1], pos[2] }
                                             : VrBeltThrow::Vec3{ 1.0e6f, 1.0e6f, 1.0e6f };
    return h;
}

} // namespace

int VrBelt::SwordHand() {
    return CVarGetInteger("gVrLeftHanded", 0) ? VR_HAND_LEFT : VR_HAND_RIGHT;
}

bool VrBelt::OffHandTriggerHeld() {
    return (VR_GetControllerButton(SwordHand() ^ 1) & VR_BTN_TRIGGER) != 0;
}

float VrBelt::UnitsPerMeter() {
    const float ws = VR_GetWorldScale();
    return (ws < 1.0f) ? 35.0f : ws;
}

bool VrBelt::Blocked(Player* player) {
    return (player->stateFlags1 & kBlockedFlags) != 0;
}

void VrBelt::Feed(VrBeltThrow::Holder& holder) {
    for (int hand = 0; hand < 2; hand++) {
        const VrCombat::TickPath& path = VrCombat::GetTickPath(hand);
        if (path.count == 0) {
            holder.Clear(hand);
        }
        for (int i = 0; i < path.count; i++) {
            const VrHandSample& s = path.samples[i];
            holder.AddSample(hand, { { s.pos[0], s.pos[1], s.pos[2] },
                                     { s.linVelMps[0], s.linVelMps[1], s.linVelMps[2] },
                                     s.timeNs });
        }
    }
}

VrBeltThrow::Input VrBelt::MakeInput(Player* player, bool beltReady, bool carrying) {
    VrBeltThrow::Input in;
    in.beltReady = beltReady;
    in.carrying = carrying;
    in.swapChord = VrItemSelect_SwapChordHeld();
    in.hands[VR_HAND_LEFT] = Hand(VR_HAND_LEFT);
    in.hands[VR_HAND_RIGHT] = Hand(VR_HAND_RIGHT);
    in.beltPos = BeltPos(player);
    in.grabRadius = VrBeltThrow::kGrabRadiusM * UnitsPerMeter();
    in.firstHand = SwordHand();
    return in;
}

VrBeltThrow::Vec3 VrBelt::BeltPos(Player* player) {
    float eye[3];
    float fwd[3];
    float up[3];
    VR_GetCameraPose(eye, fwd, up);
    const float yaw = player->actor.shape.rot.y * (M_PI / 0x8000);
    return VrBeltThrow::BeltAnchor({ eye[0], eye[1], eye[2] }, yaw, UnitsPerMeter());
}

bool VrBelt::PosInHand(int hand, float palmOffsetM, float* outXyz) {
    float quat[4];
    if (!VR_GetHandPose(hand, outXyz, quat)) {
        return false;
    }
    const VrBeltThrow::Vec3 o =
        VrBeltThrow::PalmOffset(quat, hand == VR_HAND_LEFT, UnitsPerMeter(), palmOffsetM);
    outXyz[0] += o.x;
    outXyz[1] += o.y;
    outXyz[2] += o.z;
    return true;
}

void VrBelt::Haptics(const VrBeltThrow::Result& r, bool inReach[2]) {
    for (int hand = 0; hand < 2; hand++) {
        inReach[hand] = r.inReach[hand];
        if (r.reachPulse[hand]) {
            VR_TriggerHaptic(hand, 0.25f, 0.0f, 15.0f);
        }
    }
    if (r.event == VrBeltThrow::Event::Grab) {
        VR_TriggerHaptic(r.hand, 0.5f, 0.0f, 35.0f);
    }
}
