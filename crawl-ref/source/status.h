/**
 * @file
 * @brief Collects information for output of status effects.
**/

#ifndef STATUS_H
#define STATUS_H

#include "database.h"

enum status_type
{
    STATUS_AIRBORNE = NUM_DURATIONS + 1,
    STATUS_BEHELD,
    STATUS_CONTAMINATION,
    STATUS_NET,
    STATUS_HUNGER,
    STATUS_REGENERATION,
    STATUS_ROT,
    STATUS_DIG,
    STATUS_SICK,
    STATUS_SPEED,
    STATUS_STR_ZERO,
    STATUS_INT_ZERO,
    STATUS_DEX_ZERO,
    STATUS_FIREBALL,
    STATUS_BACKLIT,
    STATUS_UMBRA,
    STATUS_CONSTRICTED,
    STATUS_MANUAL,
    STATUS_AUGMENTED,
    STATUS_TERRAIN,
    STATUS_SILENCE,
    STATUS_MISSILES,
#if TAG_MAJOR_VERSION == 34
    STATUS_NO_CTELE,
#endif
    STATUS_BEOGH,
    STATUS_RECALL,
    STATUS_LIQUEFIED,
    STATUS_DRAINED,
    STATUS_RAY,
    STATUS_ELIXIR,
    STATUS_INVISIBLE,
    STATUS_MAGIC_SAPPED,
#if TAG_MAJOR_VERSION == 34
    STATUS_GOLDEN,
#endif
    STATUS_BRIBE,
    STATUS_CLOUD,
    STATUS_BONE_ARMOUR,
    STATUS_ORB,
    STATUS_DIVINE_ENERGY,
    STATUS_STILL_WINDS,
    STATUS_LAST_STATUS = STATUS_STILL_WINDS
};

struct status_info
{
    int light_colour;
    string light_text; // status light
    string short_text; // @: line
    string long_text;  // @ message
};

// status should be a duration or status_type
// *info will be filled in as appropriate for current
// character state
// returns true if the status has a description
bool fill_status_info(int status, status_info* info);

const char *duration_name(duration_type dur);
static inline string duration_name_j(const string &en)
{
    return tagged_jtrans("[dur]", en);
}
static inline string duration_name_j(duration_type dur)
{
    return duration_name_j(duration_name(dur));
}
#define duration_name_jc(dur) (duration_name_j(dur).c_str())

bool duration_dispellable(duration_type dur);
void init_duration_index();

bool duration_decrements_normally(duration_type dur);
const char *duration_end_message(duration_type dur);
void duration_end_effect(duration_type dur);
const char *duration_mid_message(duration_type dur);
int duration_mid_offset(duration_type dur);
int duration_expire_point(duration_type dur);
msg_channel_type duration_mid_chan(duration_type dur);

#endif
