/**
 * @file
 * @brief Functions used to print information about various game objects.
**/

#include "AppHdr.h"

#include "describe.h"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <numeric>
#include <set>
#include <sstream>
#include <string>

#include "adjust.h"
#include "areas.h"
#include "art-enum.h"
#include "artefact.h"
#include "branch.h"
#include "butcher.h"
#include "cloud.h" // cloud_type_name
#include "clua.h"
#include "database.h"
#include "decks.h"
#include "delay.h"
#include "describe-spells.h"
#include "directn.h"
#include "english.h"
#include "env.h"
#include "evoke.h"
#include "fight.h"
#include "food.h"
#include "ghost.h"
#include "godabil.h"
#include "goditem.h"
#include "hints.h"
#include "invent.h"
#include "itemprop.h"
#include "items.h"
#include "item_use.h"
#include "japanese.h"
#include "jobs.h"
#include "libutil.h"
#include "macro.h"
#include "message.h"
#include "mon-book.h"
#include "mon-cast.h" // mons_spell_range
#include "mon-death.h"
#include "mon-tentacle.h"
#include "options.h"
#include "output.h"
#include "process_desc.h"
#include "prompt.h"
#include "religion.h"
#include "skills.h"
#include "species.h"
#include "spl-book.h"
#include "spl-summoning.h"
#include "spl-util.h"
#include "spl-wpnench.h"
#include "stash.h"
#include "state.h"
#include "stringutil.h" // to_string on Cygwin
#include "terrain.h"
#ifdef USE_TILE_LOCAL
 #include "tilereg-crt.h"
#endif
#include "unicode.h"

int count_desc_lines(const string &_desc, const int width)
{
    string desc = get_linebreak_string(_desc, width);
    return count(begin(desc), end(desc), '\n');
}

void print_description(const string &body)
{
    describe_info inf;
    inf.body << body;
    print_description(inf);
}

class default_desc_proc
{
public:
    int width() { return get_number_of_cols() - 1; }
    int height() { return get_number_of_lines(); }
    void print(const string &str) { cprintf("%s", str.c_str()); }

    void nextline()
    {
        if (wherey() < height())
            cgotoxy(1, wherey() + 1);
        else
            cgotoxy(1, height());
        // Otherwise cgotoxy asserts; let's just clobber the last line
        // instead, which should be noticeable enough.
    }
};

void print_description(const describe_info &inf)
{
    clrscr();
    textcolour(LIGHTGREY);

    default_desc_proc proc;
    process_description<default_desc_proc>(proc, inf);
}

static void _print_quote(const describe_info &inf)
{
    clrscr();
    textcolour(LIGHTGREY);

    default_desc_proc proc;
    process_quote<default_desc_proc>(proc, inf);
}

const char* jewellery_base_ability_string(int subtype)
{
    switch (subtype)
    {
#if TAG_MAJOR_VERSION == 34
    case RING_SUSTAIN_ATTRIBUTES: return "SustAt";
#endif
    case RING_WIZARDRY:           return "Wiz";
    case RING_FIRE:               return "Fire";
    case RING_ICE:                return "Ice";
    case RING_TELEPORTATION:      return "*Tele";
    case RING_RESIST_CORROSION:   return "rCorr";
#if TAG_MAJOR_VERSION == 34
    case RING_TELEPORT_CONTROL:   return "+cTele";
#endif
    case AMU_HARM:                return "Harm";
    case AMU_MANA_REGENERATION:   return "RegenMP";
    case AMU_THE_GOURMAND:        return "Gourm";
#if TAG_MAJOR_VERSION == 34
    case AMU_DISMISSAL:           return "Dismiss";
    case AMU_CONSERVATION:        return "Cons";
    case AMU_CONTROLLED_FLIGHT:   return "cFly";
#endif
    case AMU_GUARDIAN_SPIRIT:     return "Spirit";
    case AMU_FAITH:               return "Faith";
    case AMU_REFLECTION:          return "Reflect";
    case AMU_INACCURACY:          return "Inacc";
    }
    return "";
}

#define known_proprt(prop) (proprt[(prop)] && known[(prop)])

/// How to display props of a given type?
enum prop_note_type
{
    /// The raw numeral; e.g "Slay+3", "Int-1"
    PROPN_NUMERAL,
    /// Plusses and minuses; "rF-", "rC++"
    PROPN_SYMBOLIC,
    /// Don't note the number; e.g. "rMut"
    PROPN_PLAIN,
};

struct property_annotators
{
    artefact_prop_type prop;
    prop_note_type spell_out;
};

static vector<string> _randart_propnames(const item_def& item,
                                         bool no_comma = false)
{
    artefact_properties_t  proprt;
    artefact_known_props_t known;
    artefact_desc_properties(item, proprt, known);

    vector<string> propnames;

    // list the following in rough order of importance
    const property_annotators propanns[] =
    {
        // (Generally) negative attributes
        // These come first, so they don't get chopped off!
        { ARTP_PREVENT_SPELLCASTING,  PROPN_PLAIN },
        { ARTP_PREVENT_TELEPORTATION, PROPN_PLAIN },
        { ARTP_CONTAM,                PROPN_PLAIN },
        { ARTP_ANGRY,                 PROPN_PLAIN },
        { ARTP_CAUSE_TELEPORTATION,   PROPN_PLAIN },
        { ARTP_NOISE,                 PROPN_PLAIN },
        { ARTP_CORRODE,               PROPN_PLAIN },
        { ARTP_DRAIN,                 PROPN_PLAIN },
        { ARTP_SLOW,                  PROPN_PLAIN },
        { ARTP_FRAGILE,               PROPN_PLAIN },

        // Evokable abilities come second
        { ARTP_BLINK,                 PROPN_PLAIN },
        { ARTP_BERSERK,               PROPN_PLAIN },
        { ARTP_INVISIBLE,             PROPN_PLAIN },
        { ARTP_FLY,                   PROPN_PLAIN },
        { ARTP_FOG,                   PROPN_PLAIN },

        // Resists, also really important
        { ARTP_ELECTRICITY,           PROPN_PLAIN },
        { ARTP_POISON,                PROPN_PLAIN },
        { ARTP_FIRE,                  PROPN_SYMBOLIC },
        { ARTP_COLD,                  PROPN_SYMBOLIC },
        { ARTP_NEGATIVE_ENERGY,       PROPN_SYMBOLIC },
        { ARTP_MAGIC_RESISTANCE,      PROPN_SYMBOLIC },
        { ARTP_REGENERATION,          PROPN_SYMBOLIC },
        { ARTP_RMUT,                  PROPN_PLAIN },
        { ARTP_RCORR,                 PROPN_PLAIN },

        // Quantitative attributes
        { ARTP_HP,                    PROPN_NUMERAL },
        { ARTP_MAGICAL_POWER,         PROPN_NUMERAL },
        { ARTP_AC,                    PROPN_NUMERAL },
        { ARTP_EVASION,               PROPN_NUMERAL },
        { ARTP_STRENGTH,              PROPN_NUMERAL },
        { ARTP_INTELLIGENCE,          PROPN_NUMERAL },
        { ARTP_DEXTERITY,             PROPN_NUMERAL },
        { ARTP_SLAYING,               PROPN_NUMERAL },
        { ARTP_SHIELDING,             PROPN_NUMERAL },

        // Qualitative attributes (and Stealth)
        { ARTP_SEE_INVISIBLE,         PROPN_PLAIN },
        { ARTP_STEALTH,               PROPN_SYMBOLIC },
        { ARTP_CURSE,                 PROPN_PLAIN },
        { ARTP_CLARITY,               PROPN_PLAIN },
        { ARTP_RMSL,                  PROPN_PLAIN },
    };

    const unrandart_entry *entry = nullptr;
    if (is_unrandom_artefact(item))
        entry = get_unrand_entry(item.unrand_idx);

    // For randart jewellery, note the base jewellery type if it's not
    // covered by artefact_desc_properties()
    if (item.base_type == OBJ_JEWELLERY
        && (item_ident(item, ISFLAG_KNOW_TYPE)))
    {
        const char* type = jewellery_base_ability_string(item.sub_type);
        if (*type)
            propnames.push_back(type);
    }
    else if ((item_ident(item, ISFLAG_KNOW_TYPE)
              || is_artefact(item)
                 && artefact_known_property(item, ARTP_BRAND))
             && !(is_unrandom_artefact(item) && entry
                  && entry->flags & UNRAND_FLAG_SKIP_EGO))
    {
        string ego;
        if (item.base_type == OBJ_WEAPONS)
            ego = weapon_brand_name(item, true);
        else if (item.base_type == OBJ_ARMOUR)
            ego = armour_ego_name(item, true);
        if (!ego.empty())
        {
            // XXX: Ugly hack for adding a comma if needed.
            bool extra_props = false;
            for (const property_annotators &ann : propanns)
                if (known_proprt(ann.prop) && ann.prop != ARTP_BRAND)
                {
                    extra_props = true;
                    break;
                }

            if (!no_comma && extra_props
                || is_unrandom_artefact(item)
                   && entry && entry->inscrip != nullptr)
            {
                ego += ",";
            }

            propnames.push_back(ego);
        }
    }

    if (is_unrandom_artefact(item) && entry && entry->inscrip != nullptr)
        propnames.push_back(entry->inscrip);

    for (const property_annotators &ann : propanns)
    {
        if (known_proprt(ann.prop))
        {
            const int val = proprt[ann.prop];

            // Don't show rF+/rC- for =Fire, or vice versa for =Ice.
            if (item.base_type == OBJ_JEWELLERY)
            {
                if (item.sub_type == RING_FIRE
                    && (ann.prop == ARTP_FIRE && val == 1
                        || ann.prop == ARTP_COLD && val == -1))
                {
                    continue;
                }
                if (item.sub_type == RING_ICE
                    && (ann.prop == ARTP_COLD && val == 1
                        || ann.prop == ARTP_FIRE && val == -1))
                {
                    continue;
                }
            }

            ostringstream work;
            switch (ann.spell_out)
            {
            case PROPN_NUMERAL: // e.g. AC+4
                work << showpos << artp_name(ann.prop) << val;
                break;
            case PROPN_SYMBOLIC: // e.g. F++
            {
                work << artp_name(ann.prop);

                char symbol = val > 0 ? '+' : '-';
                const int sval = abs(val);
                if (sval > 4)
                    work << symbol << sval;
                else
                    work << string(sval, symbol);

                break;
            }
            case PROPN_PLAIN: // e.g. rPois or SInv
                if (ann.prop == ARTP_CURSE && val < 1)
                    continue;

                work << artp_name(ann.prop);
                break;
            }
            propnames.push_back(work.str());
        }
    }

    return propnames;
}

string artefact_inscription(const item_def& item)
{
    if (item.base_type == OBJ_BOOKS)
        return "";

    const vector<string> propnames = _randart_propnames(item);

    string insc = comma_separated_line(propnames.begin(), propnames.end(),
                                       " ", " ");
    if (!insc.empty() && insc[insc.length() - 1] == ',')
        insc.erase(insc.length() - 1);
    return sp2nbsp(insc);
}

void add_inscription(item_def &item, string inscrip)
{
    if (!item.inscription.empty())
    {
        if (ends_with(item.inscription, ","))
            item.inscription += " ";
        else
            item.inscription += ", ";
    }

    item.inscription += inscrip;
}

static const char* _jewellery_base_ability_description(int subtype)
{
    switch (subtype)
    {
#if TAG_MAJOR_VERSION == 34
    case RING_SUSTAIN_ATTRIBUTES:
        return "It sustains your strength, intelligence and dexterity.";
#endif
    case RING_WIZARDRY:
        return "It improves your spell success rate.";
    case RING_FIRE:
        return "It enhances your fire magic.";
    case RING_ICE:
        return "It enhances your ice magic.";
    case RING_TELEPORTATION:
        return "It may teleport you next to monsters.";
#if TAG_MAJOR_VERSION == 34
    case RING_TELEPORT_CONTROL:
        return "It can be evoked for teleport control.";
#endif
    case AMU_HARM:
        return "It increases damage dealt and taken.";
    case AMU_MANA_REGENERATION:
        return "It increases your magic regeneration.";
    case AMU_THE_GOURMAND:
        return "It allows you to eat raw meat even when not hungry.";
#if TAG_MAJOR_VERSION == 34
    case AMU_DISMISSAL:
        return "It may teleport away creatures that harm you.";
    case AMU_CONSERVATION:
        return "It protects your inventory from destruction.";
#endif
    case AMU_GUARDIAN_SPIRIT:
        return "It causes incoming damage to be split between your health and "
               "magic.";
    case AMU_FAITH:
        return "It allows you to gain divine favour quickly.";
    case AMU_REFLECTION:
        return "It shields you and reflects attacks.";
    case AMU_INACCURACY:
        return "It reduces the accuracy of all your attacks.";
    }
    return "";
}

struct property_descriptor
{
    artefact_prop_type property;
    const char* desc;           // If it contains %d, will be replaced by value.
    bool is_graded_resist;
};

static string _randart_base_type_string(const item_def &item)
{
    const string basename = base_type_string(item);
    return basename != "jewellery" ? basename
                                   : jewellery_is_amulet(item) ? "amulet"
                                                               : "ring";
}

static string _randart_descrip(const item_def &item)
{
    string description;

    artefact_properties_t  proprt;
    artefact_known_props_t known;
    artefact_desc_properties(item, proprt, known);

    const property_descriptor propdescs[] =
    {
        { ARTP_AC, "It affects your AC (%d).", false },
        { ARTP_EVASION, "It affects your evasion (%d).", false},
        { ARTP_STRENGTH, "It affects your strength (%d).", false},
        { ARTP_INTELLIGENCE, "It affects your intelligence (%d).", false},
        { ARTP_DEXTERITY, "It affects your dexterity (%d).", false},
        { ARTP_SLAYING, "It affects your accuracy and damage with ranged "
                        "weapons and melee attacks (%d).", false},
        { ARTP_FIRE, "fire", true},
        { ARTP_COLD, "cold", true},
        { ARTP_ELECTRICITY, "It insulates you from electricity.", false},
        { ARTP_POISON, "poison", true},
        { ARTP_NEGATIVE_ENERGY, "negative energy", true},
        { ARTP_MAGIC_RESISTANCE, "It affects your resistance to hostile "
                                 "enchantments.", false},
        { ARTP_HP, "It affects your health (%d).", false},
        { ARTP_MAGICAL_POWER, "It affects your magic capacity (%d).", false},
        { ARTP_SEE_INVISIBLE, "It lets you see invisible.", false},
        { ARTP_INVISIBLE, "It lets you turn invisible.", false},
        { ARTP_FLY, "It lets you fly.", false},
        { ARTP_BLINK, "It lets you blink.", false},
        { ARTP_BERSERK, "It lets you go berserk.", false},
        { ARTP_NOISE, "It may make noises in combat.", false},
        { ARTP_PREVENT_SPELLCASTING, "It prevents spellcasting.", false},
        { ARTP_CAUSE_TELEPORTATION, "It may teleport you next to monsters.", false},
        { ARTP_PREVENT_TELEPORTATION, "It prevents most forms of teleportation.",
          false},
        { ARTP_ANGRY,  "It may make you go berserk in combat.", false},
        { ARTP_CURSE, "It may re-curse itself when equipped.", false},
        { ARTP_CLARITY, "It protects you against confusion.", false},
        { ARTP_CONTAM, "It causes magical contamination when unequipped.", false},
        { ARTP_RMSL, "It protects you from missiles.", false},
        { ARTP_FOG, "It can be evoked to emit clouds of fog.", false},
        { ARTP_REGENERATION, "It increases your rate of regeneration.", false},
        { ARTP_RCORR, "It protects you from acid and corrosion.", false},
        { ARTP_RMUT, "It protects you from mutation.", false},
        { ARTP_CORRODE, "It may corrode you when you take damage.", false},
        { ARTP_DRAIN, "It causes draining when unequipped.", false},
        { ARTP_SLOW, "It may slow you when you take damage.", false},
        { ARTP_FRAGILE, "It will be destroyed if unequipped.", false },
        { ARTP_SHIELDING, "It affects your SH (%d).", false},
    };

    // Give a short description of the base type, for base types with no
    // corresponding ARTP.
    if (item.base_type == OBJ_JEWELLERY
        && (item_ident(item, ISFLAG_KNOW_TYPE)))
    {
        const char* type = _jewellery_base_ability_description(item.sub_type);
        if (*type)
        {
            description += "\n";
            description += make_stringf(jtransc(type),
                                        jtransc(item.name(DESC_BASENAME)));
        }
    }

    for (const property_descriptor &desc : propdescs)
    {
        if (known_proprt(desc.property))
        {
            // Only randarts with ARTP_CURSE > 0 may recurse themselves.
            if (desc.property == ARTP_CURSE && proprt[desc.property] < 1)
                continue;

            string sdesc = sp2nbsp(jtrans(desc.desc));

            // FIXME Not the nicest hack.
            char buf[80];
            snprintf(buf, sizeof buf, "%+d", proprt[desc.property]);
            sdesc = replace_all(sdesc, "%d", buf);

            if (desc.is_graded_resist)
            {
                int idx = proprt[desc.property] + 3;
                idx = min(idx, 6);
                idx = max(idx, 0);

                const char* prefixes[] =
                {
                    "It makes you extremely vulnerable to ",
                    "It makes you very vulnerable to ",
                    "It makes you vulnerable to ",
                    "Buggy descriptor!",
                    "It protects you from ",
                    "It greatly protects you from ",
                    "It renders you almost immune to "
                };
                sdesc = make_stringf(jtransc(prefixes[idx]),
                                     sdesc.c_str()) + jtrans(".");
            }

            sdesc = make_stringf(sdesc.c_str(),
                                 jtransc(_randart_base_type_string(item)));
            description += '\n';
            description += sdesc;
        }
    }

    if (known_proprt(ARTP_STEALTH))
    {
        const int stval = proprt[ARTP_STEALTH];
        char buf[80];
        snprintf(buf, sizeof buf, "\nIt makes you %s%s stealthy.",
                 (stval < -1 || stval > 1) ? "much " : "",
                 (stval < 0) ? "less" : "more");
        description += make_stringf(jtrans_notrimc(buf),
                                    jtransc(_randart_base_type_string(item)));
    }

    return description;
}
#undef known_proprt

static const char *trap_names[] =
{
#if TAG_MAJOR_VERSION == 34
    "dart",
#endif
    "arrow", "spear",
#if TAG_MAJOR_VERSION > 34
    "teleport",
#endif
    "permanent teleport",
    "alarm", "blade",
    "bolt", "net", "Zot", "needle",
    "shaft", "passage", "pressure plate", "web",
#if TAG_MAJOR_VERSION == 34
    "gas", "teleport",
    "shadow", "dormant shadow",
#endif
};

string trap_name(trap_type trap)
{
    COMPILE_CHECK(ARRAYSZ(trap_names) == NUM_TRAPS);

    if (trap >= 0 && trap < NUM_TRAPS)
        return trap_names[trap];
    return "";
}

string full_trap_name(trap_type trap)
{
    string basename = trap_name(trap);
    switch (trap)
    {
    case TRAP_GOLUBRIA:
        return basename + " of Golubria";
    case TRAP_PLATE:
    case TRAP_WEB:
    case TRAP_SHAFT:
        return basename;
    default:
        return basename + " trap";
    }
}

int str_to_trap(const string &s)
{
    // "Zot trap" is capitalised in trap_names[], but the other trap
    // names aren't.
    const string tspec = lowercase_string(s);

    // allow a couple of synonyms
    if (tspec == "random" || tspec == "any")
        return TRAP_RANDOM;

    for (int i = 0; i < NUM_TRAPS; ++i)
        if (tspec == lowercase_string(trap_names[i]))
            return i;

    return -1;
}

/**
 * How should this panlord be described?
 *
 * @param name   The panlord's name; used as a seed for its appearance.
 * @param flying Whether the panlord can fly.
 * @returns a string including a description of its head, its body, its flight
 *          mode (if any), and how it smells or looks.
 */
static string _describe_demon(const string& name, bool flying)
{
    const uint32_t seed = hash32(&name[0], name.size());
    #define HRANDOM_ELEMENT(arr, id) arr[hash_rand(ARRAYSZ(arr), seed, id)]

    static const char* body_types[] =
    {
        "armoured",
        "vast, spindly",
        "fat",
        "obese",
        "muscular",
        "spiked",
        "splotchy",
        "slender",
        "tentacled",
        "emaciated",
        "bug-like",
        "skeletal",
        "mantis",
    };

    static const char* wing_names[] =
    {
        "with small, bat-like wings",
        "with bony wings",
        "with sharp, metallic wings",
        "with the wings of a moth",
        "with thin, membranous wings",
        "with dragonfly wings",
        "with large, powerful wings",
        "with fluttering wings",
        "with great, sinister wings",
        "with hideous, tattered wings",
        "with sparrow-like wings",
        "with hooked wings",
        "with strange knobs attached",
        "which hovers in mid-air",
        "with sacs of gas hanging from its back",
    };

    const char* head_names[] =
    {
        "a cubic structure in place of a head",
        "a brain for a head",
        "a hideous tangle of tentacles for a mouth",
        "the head of an elephant",
        "an eyeball for a head",
        "wears a helmet over its head",
        "a horn in place of a head",
        "a thick, horned head",
        "the head of a horse",
        "a vicious glare",
        "snakes for hair",
        "the face of a baboon",
        "the head of a mouse",
        "a ram's head",
        "the head of a rhino",
        "eerily human features",
        "a gigantic mouth",
        "a mass of tentacles growing from its neck",
        "a thin, worm-like head",
        "huge, compound eyes",
        "the head of a frog",
        "an insectoid head",
        "a great mass of hair",
        "a skull for a head",
        "a cow's skull for a head",
        "the head of a bird",
        "a large fungus growing from its neck",
    };

    static const char* misc_descs[] =
    {
        " It seethes with hatred of the living.",
        " Tiny orange flames dance around it.",
        " Tiny purple flames dance around it.",
        " It is surrounded by a weird haze.",
        " It glows with a malevolent light.",
        " It looks incredibly angry.",
        " It oozes with slime.",
        " It dribbles constantly.",
        " Mould grows all over it.",
        " Its body is covered in fungus.",
        " It is covered with lank hair.",
        " It looks diseased.",
        " It looks as frightened of you as you are of it.",
        " It moves in a series of hideous convulsions.",
        " It moves with an unearthly grace.",
        " It leaves a glistening oily trail.",
        " It shimmers before your eyes.",
        " It is surrounded by a brilliant glow.",
        " It radiates an aura of extreme power.",
        " It seems utterly heartbroken.",
        " It seems filled with irrepressible glee.",
        " It constantly shivers and twitches.",
        " Blue sparks crawl across its body.",
        " It seems uncertain.",
        " A cloud of flies swarms around it.",
        " The air around it ripples with heat.",
        " Crystalline structures grow on everything near it.",
        " It appears supremely confident.",
        " Its skin is covered in a network of cracks.",
        " Its skin has a disgusting oily sheen.",
        " It seems somehow familiar.",
        " It is somehow always in shadow.",
        " It is difficult to look away.",
        " It is constantly speaking in tongues.",
        " It babbles unendingly.",
        " Its body is scourged by damnation.",
        " Its body is extensively scarred.",
        " You find it difficult to look away.",
    };

    static const char* smell_descs[] =
    {
        " It smells of brimstone.",
        " It is surrounded by a sickening stench.",
        " It smells of rotting flesh.",
        " It stinks of death.",
        " It stinks of decay.",
        " It smells delicious!",
    };

    ostringstream description;
    description << make_stringf(jtransc("One of the many lords of Pandemonium, {name} has "),
                                name.c_str());

    description << jtrans(HRANDOM_ELEMENT(body_types, 2));
    description << jtrans(" body ");

    const string head_desc = jtrans(HRANDOM_ELEMENT(head_names, 1));

    if (flying)
    {
        string wing_text = jtrans(HRANDOM_ELEMENT(wing_names, 3));
        if (!ends_with(wing_text, "、"))
            description << ( "と" + wing_text + "を持ち、");
        else
            description << wing_text;
    }
    else
        description << "で、";

    description << head_desc;
    if (head_desc.find("。") == string::npos)
        description << "を持っている。";

    if (!hash_rand(5, seed, 4) && you.can_smell()) // 20%
    {
        description << "\n";
        description << jtrans(HRANDOM_ELEMENT(smell_descs, 5));
    }

    if (hash_rand(2, seed, 6)) // 50%
    {
        description << "\n";
        description << jtrans(HRANDOM_ELEMENT(misc_descs, 6));
    }

    return description.str();
}

/**
 * Describe a given mutant beast's tier.
 *
 * @param tier      The mutant_beast_tier of the beast in question.
 * @return          A string describing the tier; e.g.
 *              "It is a juvenile, out of the larval stage but still below its
 *              mature strength."
 */
static string _describe_mutant_beast_tier(int tier)
{
    static const string tier_descs[] = {
        "It is of an unusually buggy age.",
        "It is larval and weak, freshly emerged from its mother's pouch.",
        "It is a juvenile, no longer larval but below its mature strength.",
        "It is mature, stronger than a juvenile but weaker than its elders.",
        "It is an elder, stronger than mature beasts.",
        "It is a primal beast, the most powerful of its kind.",
    };
    COMPILE_CHECK(ARRAYSZ(tier_descs) == NUM_BEAST_TIERS);

    ASSERT_RANGE(tier, 0, NUM_BEAST_TIERS);
    return tier_descs[tier];
}


/**
 * Describe a given mutant beast's facets.
 *
 * @param facets    A vector of the mutant_beast_facets in question.
 * @return          A string describing the facets; e.g.
 *              "It flies and flits around unpredictably, and its breath
 *               smoulders ominously."
 */
static string _describe_mutant_beast_facets(const CrawlVector &facets)
{
    static const string facet_descs[] = {
        " seems unusually buggy.",
        " sports a set of venomous tails",
        " flies swiftly and unpredictably",
        "s breath smoulders ominously",
        " is covered with eyes and tentacles",
        " flickers and crackles with electricity",
        " is covered in dense fur and muscle",
    };
    COMPILE_CHECK(ARRAYSZ(facet_descs) == NUM_BEAST_FACETS);

    if (facets.size() == 0)
        return "";

    const string sep = "\n" + jtrans("facet list prefix");

    return make_stringf(jtransc("It {facet list}"),
           comma_separated_fn(begin(facets), end(facets),
                      [] (const CrawlStoreValue &sv) -> string {
                          const int facet = sv.get_int();
                          ASSERT_RANGE(facet, 0, NUM_BEAST_FACETS);
                          return jtrans(facet_descs[facet]);
                      }, sep, sep).c_str());
}

/**
 * Describe a given mutant beast's special characteristics: its tier & facets.
 *
 * @param mi    The player-visible information about the monster in question.
 * @return      A string describing the monster; e.g.
 *              "It is a juvenile, out of the larval stage but still below its
 *              mature strength. It flies and flits around unpredictably, and
 *              its breath has a tendency to ignite when angered."
 */
static string _describe_mutant_beast(const monster_info &mi)
{
    const int xl = mi.props[MUTANT_BEAST_TIER].get_short();
    const int tier = mutant_beast_tier(xl);
    const CrawlVector facets = mi.props[MUTANT_BEAST_FACETS].get_vector();
    return _describe_mutant_beast_facets(facets)
           + "\n" + jtrans(_describe_mutant_beast_tier(tier));
}

static void _append_weapon_stats(string &description, const item_def &item)
{
    const int base_dam = property(item, PWPN_DAMAGE);
    const int ammo_type = fires_ammo_type(item);
    const int ammo_dam = ammo_type == MI_NONE ? 0 :
                                                ammo_type_damage(ammo_type);
    const skill_type skill = item_attack_skill(item);

    const string your_skill = crawl_state.need_save ?
      make_stringf(jtrans_notrimc("\n (Your skill: %.1f)"), (float) you.skill(skill, 10) / 10)
      : "";
    description += make_stringf((
    jtrans_notrim("\nBase accuracy: %+d  Base damage: %d  Base attack delay: %.1f") +
    jtrans_notrim("\nThis weapon's minimum attack delay (%.1f) is reached at skill level %d.") +
    "%s").c_str(),
     property(item, PWPN_HIT),
     base_dam + ammo_dam,
     (float) property(item, PWPN_SPEED) / 10,
     weapon_min_delay_skill(item),
     (float) weapon_min_delay(item) / 10,
     your_skill.c_str());

    if (skill == SK_SLINGS)
    {
        description += make_stringf(jtrans_notrimc("\nFiring bullets:    Base damage: %d"),
                                    base_dam +
                                    ammo_type_damage(MI_SLING_BULLET));
    }
}

static string _handedness_string(const item_def &item)
{
    string description;

    switch (you.hands_reqd(item))
    {
    case HANDS_ONE:
        if (you.species == SP_FORMICID)
            description += "It is a weapon for one hand-pair.";
        else
            description += "It is a one handed weapon.";
        break;
    case HANDS_TWO:
        description += "It is a two handed weapon.";
        break;
    }

    return description;
}

static string _describe_weapon(const item_def &item, bool verbose)
{
    string description;

    description.reserve(200);

    description = "";

    if (verbose)
    {
        description += "\n";
        _append_weapon_stats(description, item);
    }

    const int spec_ench = (is_artefact(item) || verbose)
                          ? get_weapon_brand(item) : SPWPN_NORMAL;
    const int damtype = get_vorpal_type(item);

    if (verbose)
    {
        switch (item_attack_skill(item))
        {
        case SK_POLEARMS:
            description += jtrans_notrim("\n\nIt can be evoked to extend its reach.");
            break;
        case SK_AXES:
            description += jtrans_notrim("\n\nIt hits all enemies adjacent to the wielder, "
                           "dealing less damage to those not targeted.");
            break;
        case SK_LONG_BLADES:
            description += jtrans_notrim("\n\nIt can be used to riposte, swiftly "
                           "retaliating against a missed attack.");
            break;
        case SK_SHORT_BLADES:
            {
                string adj = (item.sub_type == WPN_DAGGER) ? "extremely"
                                                           : "particularly";
                description += make_stringf(jtrans_notrimc("\n\nIt is {adj} good for stabbing"
                                                           " unaware enemies."),
                                            adj_jc(adj));
            }
            break;
        default:
            break;
        }
    }

    // ident known & no brand but still glowing
    // TODO: deduplicate this with the code in itemname.cc
    const bool enchanted = get_equip_desc(item) && spec_ench == SPWPN_NORMAL
                           && !item_ident(item, ISFLAG_KNOW_PLUSES);

    // special weapon descrip
    if (item_type_known(item) && (spec_ench != SPWPN_NORMAL || enchanted))
    {
        string special_weapon_description;

        switch (spec_ench)
        {
        case SPWPN_FLAMING:
            if (is_range_weapon(item))
            {
                special_weapon_description += jtrans("It causes projectiles fired from it to burn "
                    "those they strike,");
            }
            else
            {
                special_weapon_description += jtrans("It has been specially enchanted to burn "
                    "those struck by it,");
            }
            special_weapon_description += jtrans("causing extra injury flaming");
            if (!is_range_weapon(item) &&
                (damtype == DVORP_SLICING || damtype == DVORP_CHOPPING))
            {
                special_weapon_description += jtrans_notrim("\nBig, fiery blades are also staple "
                    "armaments of hydra-hunters.");
            }
            break;
        case SPWPN_FREEZING:
            if (is_range_weapon(item))
            {
                special_weapon_description += jtrans("It causes projectiles fired from it to freeze "
                    "those they strike,");
            }
            else
            {
                special_weapon_description += jtrans("It has been specially enchanted to freeze "
                    "those struck by it,");
            }
            special_weapon_description += jtrans("causing extra injury freezing");
            special_weapon_description += jtrans_notrim(make_stringf("\n%s can also slow down cold-blooded creatures.",
                                                      is_range_weapon(item) ? "They" : "It"));
            break;
        case SPWPN_HOLY_WRATH:
            special_weapon_description += jtrans("It has been blessed by the Shining One");
            if (is_range_weapon(item))
            {
                special_weapon_description += make_stringf(jtransc(", and any {ammo name} fired from it will"),
                                            ammo_name_jc(item));
            }
            special_weapon_description += jtrans(" cause great damage to the undead and demons.");
            break;
        case SPWPN_ELECTROCUTION:
            if (is_range_weapon(item))
            {
                special_weapon_description += jtrans("It charges the ammunition it shoots with "
                    "electricity; occasionally upon a hit, such missiles "
                    "may discharge and cause terrible harm.");
            }
            else
            {
                special_weapon_description += jtrans("Occasionally, upon striking a foe, it will "
                    "discharge some electrical energy and cause terrible "
                    "harm.");
            }
            break;
        case SPWPN_VENOM:
            if (is_range_weapon(item))
                special_weapon_description += jtrans("It poisons the ammo it fires.");
            else
                special_weapon_description += jtrans("It poisons the flesh of those it strikes.");
            break;
        case SPWPN_PROTECTION:
            special_weapon_description += jtrans("It protects the one who uses it against "
                "injury (+AC on strike).");
            break;
        case SPWPN_DRAINING:
            special_weapon_description += jtrans("A truly terrible weapon, it drains the "
                "life of those it strikes.");
            break;
        case SPWPN_SPEED:
            special_weapon_description += jtrans("Attacks with this weapon are significantly faster.");
            break;
        case SPWPN_VORPAL:
            if (is_range_weapon(item))
            {
                special_weapon_description += make_stringf(jtransc("Any {ammo name} fired from it inflicts extra damage."),
                                            ammo_name_jc(item));
            }
            else
            {
                special_weapon_description += jtrans("It inflicts extra damage upon your "
                    "enemies.");
            }
            break;
        case SPWPN_CHAOS:
            if (is_range_weapon(item))
            {
                special_weapon_description += jtrans("Each projectile launched from it has a "
                                      "different, random effect.");
            }
            else
            {
                special_weapon_description += jtrans("Each time it hits an enemy it has a "
                    "different, random effect.");
            }
            break;
        case SPWPN_VAMPIRISM:
            special_weapon_description += jtrans("It inflicts no extra harm, but heals "
                "its wielder when it wounds a living foe.");
            break;
        case SPWPN_PAIN:
            special_weapon_description += jtrans("In the hands of one skilled in necromantic "
                "magic, it inflicts extra damage on living creatures.");
            break;
        case SPWPN_DISTORTION:
            special_weapon_description += jtrans("It warps and distorts space around it. "
                "Unwielding it can cause banishment or high damage.");
            break;
        case SPWPN_PENETRATION:
            special_weapon_description += jtrans("Ammo fired by it will pass through the "
                "targets it hits, potentially hitting all targets in "
                "its path until it reaches maximum range.");
            break;
        case SPWPN_REAPING:
            special_weapon_description += jtrans("If a monster killed with it leaves a "
                "corpse in good enough shape, the corpse will be "
                "animated as a zombie friendly to the killer.");
            break;
        case SPWPN_ANTIMAGIC:
            special_weapon_description += jtrans("It reduces the magical energy of the wielder, "
                    "and disrupts the spells and magical abilities of those "
                    "hit. Natural abilities and divine invocations are not "
                    "affected.");
            break;
        case SPWPN_NORMAL:
            ASSERT(enchanted);
            special_weapon_description += jtrans("It has no special brand (it is not flaming, "
                    "freezing, etc), but is still enchanted in some way - "
                    "positive or negative.");
            break;
        }

        if (!special_weapon_description.empty())
            description += "\n\n" + special_weapon_description;
    }

    if (you.duration[DUR_EXCRUCIATING_WOUNDS] && &item == you.weapon())
    {
        description += jtrans_notrim("\nIt is temporarily rebranded; it is actually a");
        if ((int) you.props[ORIGINAL_BRAND_KEY] == SPWPN_NORMAL)
            description += jtrans("n unbranded weapon.");
        else
        {
            description += make_stringf(jtransc(" weapon of {ego type}."),
                                        ego_type_string(item, false, you.props[ORIGINAL_BRAND_KEY]).c_str());
        }
    }

    if (is_artefact(item))
    {
        string rand_desc = _randart_descrip(item);
        if (!rand_desc.empty())
        {
            if (!description.empty())
                description += "\n";
            description += rand_desc;
        }

        // XXX: Can't happen, right?
        if (!item_ident(item, ISFLAG_KNOW_PROPERTIES)
            && item_type_known(item))
        {
            description += jtrans_notrim("\nThis weapon may have some hidden properties.");
        }
    }

    if (verbose)
    {
        const skill_type skill = item_attack_skill(item);
        const string text = make_stringf(jtrans_notrimc("\n\nThis {weapon name} falls into the '{skill}' category."),
                                         jtransc(is_unrandom_artefact(item) ? get_artefact_base_name(item).c_str()
                                                                            : "weapon"),
                                         skill == SK_FIGHTING ? "buggy" : skill_name_jc(skill));

        description += text;
        description += jtrans(_handedness_string(item));

        if (!you.could_wield(item, true) && crawl_state.need_save)
            description += jtrans_notrim("\nIt is too large for you to wield.");
    }

    if (!is_artefact(item))
    {
        if (item_ident(item, ISFLAG_KNOW_PLUSES) && item.plus >= MAX_WPN_ENCHANT)
            description += jtrans_notrim("\nIt cannot be enchanted further.");
        else
        {
            description += make_stringf(jtrans_notrimc("\nIt can be maximally enchanted to +%s."),
                                        to_stringc(MAX_WPN_ENCHANT));
        }
    }

    return sp2nbsp(description);
}

static string _describe_ammo(const item_def &item)
{
    string description;

    description.reserve(64);

    const bool can_launch = has_launcher(item);
    const bool can_throw  = is_throwable(&you, item, true);
    const string basename = ammo_name_j(static_cast<missile_type>(item.sub_type));

    if (item.brand && item_type_known(item))
    {
        description += "\n\n";

        switch (item.brand)
        {
#if TAG_MAJOR_VERSION == 34
        case SPMSL_FLAME:
            description += "It burns those it strikes, causing extra injury "
                    "to most foes and up to half again as much damage against "
                    "particularly susceptible opponents. Compared to normal "
                    "ammo, it is twice as likely to be destroyed on impact.";
            break;
        case SPMSL_FROST:
            description += "It freezes those it strikes, causing extra injury "
                    "to most foes and up to half again as much damage against "
                    "particularly susceptible opponents. It can also slow down "
                    "cold-blooded creatures. Compared to normal ammo, it is "
                    "twice as likely to be destroyed on impact.";
            break;
#endif
        case SPMSL_CHAOS:
        {
            description += make_stringf(jtransc("When {thrown, (or fired from an appropriate launcher,)} "
                                                "it has a random effect."),
                                        basename.c_str(),
                                        jtransc(can_throw ? (can_launch ? "thrown, or fired from an appropriate launcher,"
                                                                        : "thrown, ")
                                                          : ""));
            break;
        }
        case SPMSL_POISONED:
            description += make_stringf(jtransc("It is coated with poison."),
                                        basename.c_str());
            break;
        case SPMSL_CURARE:
            description += make_stringf(jtransc(
                           "It is tipped with a substance that causes "
                           "asphyxiation, dealing direct damage as well as "
                           "poisoning and slowing those it strikes.\n"
                           "It is twice as likely to be destroyed on impact as "
                           "other needles."),
                                        basename.c_str());
            break;
        case SPMSL_PARALYSIS:
            description += make_stringf(jtransc("It is tipped with a paralysing substance."),
                                        basename.c_str());
            break;
        case SPMSL_SLEEP:
            description += make_stringf(jtransc("It is coated with a fast-acting tranquilizer."),
                                        basename.c_str());
            break;
        case SPMSL_CONFUSION:
            description += make_stringf(jtransc("It is tipped with a substance that causes confusion."),
                                        basename.c_str());
            break;
#if TAG_MAJOR_VERSION == 34
        case SPMSL_SICKNESS:
            description += make_stringf(jtransc("It has been contaminated by something likely to cause disease."),
                                        basename.c_str());
            break;
#endif
        case SPMSL_FRENZY:
            description += make_stringf(jtransc(
                           "It is tipped with a substance that sends those it "
                           "hits into a mindless rage, attacking friend and "
                           "foe alike."),
                                        basename.c_str());
            break;
        case SPMSL_RETURNING:
            description += make_stringf(jtransc(
                           "A skilled user can throw it in such a way that it "
                           "will return to its owner."),
                                        basename.c_str());
            break;
        case SPMSL_PENETRATION:
            description += make_stringf(jtransc(
                           "It will pass through any targets it hits, "
                           "potentially hitting all targets in its path until "
                           "it reaches its maximum range."),
                                        basename.c_str());
            break;
        case SPMSL_DISPERSAL:
            description += make_stringf(jtransc(
                           "It will cause any target it hits to blink, with a "
                           "tendency towards blinking further away from the "
                           "one who (threw or fired) it."),
                                        basename.c_str());
            break;
        case SPMSL_EXPLODING:
            description += make_stringf(jtransc(
                           "It will explode into fragments upon hitting a "
                           "target, hitting an obstruction, or reaching its "
                           "maximum range."),
                                        basename.c_str());
            break;
        case SPMSL_STEEL:
            description += make_stringf(jtransc("It deals increased damage compared to normal ammo."),
                                        basename.c_str());
            break;
        case SPMSL_SILVER:
            description += make_stringf(jtransc(
                           "It deals substantially increased damage to chaotic "
                           "and magically transformed beings. It also inflicts "
                           "extra damage against mutated beings, according to "
                           "how mutated they are."),
                                        basename.c_str());
            break;
        }
    }

    const int dam = property(item, PWPN_DAMAGE);
    if (dam)
    {
        if (!ends_with(description, "\n\n")) description += "\n";

        const int throw_delay = (10 + dam / 2);
        const string your_skill = crawl_state.need_save ?
                make_stringf(jtrans_notrimc("\n (Your skill: %.1f)"),
                    (float) you.skill(SK_THROWING, 10) / 10)
                    : "";

        description += make_stringf(jtrans_notrimc(
            "\nBase damage: %d  Base attack delay: %.1f"
            "\nThis projectile's minimum attack delay (%.1f) "
                "is reached at skill level %d."
            "%s\n"),
            dam,
            (float) throw_delay / 10,
            (throw_delay - FASTEST_PLAYER_THROWING_SPEED) * 2,
            (float) FASTEST_PLAYER_THROWING_SPEED / 10,
            your_skill.c_str()
        );
    }

    if (ammo_always_destroyed(item))
    {
        if (description.empty()) description += "\n";
        description += make_stringf(jtrans_notrimc("\nIt will always be destroyed on impact."),
                                    basename.c_str());
    }
    else if (!ammo_never_destroyed(item))
    {
        if (description.empty()) description += "\n";
        description += make_stringf(jtrans_notrimc("\nIt may be destroyed on impact."),
                                    basename.c_str());
    }

    return description;
}

static string _warlock_mirror_reflect_desc()
{
    const int SH = crawl_state.need_save ? player_shield_class() : 0;
    const int reflect_chance = 100 * SH / omnireflect_chance_denom(SH);
    return make_stringf(jtrans_notrimc("\n\nWith your current SH, it has a {reflect chance}"
           "% chance to reflect enchantments and other normally unblockable "
           "effects."),
                        reflect_chance);
}

static string _describe_armour(const item_def &item, bool verbose)
{
    string description;

    description.reserve(200);

    if (verbose)
    {
        if (is_shield(item))
        {
            const float skill = you.get_shield_skill_to_offset_penalty(item);
            description += jtrans_notrim("\nBase shield rating: ")
                        + to_string(property(item, PARM_AC));
            if (!is_useless_item(item))
            {
                description += jtrans_notrim("       Skill to remove penalty: ")
                            + make_stringf("%.1f", skill);
                if (crawl_state.need_save)
                {
                    description += "\n                            "
                                + make_stringf(jtransc("(Your skill: %.1f)"),
                                               (float) you.skill(SK_SHIELDS, 10) / 10);
                }
                else
                    description += "\n";
            }

            if (is_unrandom_artefact(item, UNRAND_WARLOCK_MIRROR))
                description += _warlock_mirror_reflect_desc();
        }
        else
        {
            const int evp = property(item, PARM_EVASION);
            description += jtrans_notrim("\n\nBase armour rating: ")
                        + to_string(property(item, PARM_AC));
            if (get_armour_slot(item) == EQ_BODY_ARMOUR)
            {
                description += jtrans_notrim("       Encumbrance rating: ")
                            + to_string(-evp / 10);
            }
            // Bardings reduce evasion by a fixed amount, and don't have any of
            // the other effects of encumbrance.
            else if (evp)
            {
                description += jtrans_notrim("       Evasion: ")
                            + to_string(evp / 30);
            }

            // only display player-relevant info if the player exists
            if (crawl_state.need_save && get_armour_slot(item) == EQ_BODY_ARMOUR)
            {
                description += make_stringf(jtrans_notrimc("\nWearing mundane armour of this type "
                                            "will give the following: %d AC"),
                                             you.base_ac_from(item));
            }
        }
    }

    const int ego = get_armour_ego_type(item);
    const bool enchanted = get_equip_desc(item) && ego == SPARM_NORMAL
                           && !item_ident(item, ISFLAG_KNOW_PLUSES);

    if ((ego != SPARM_NORMAL || enchanted) && item_type_known(item) && verbose)
    {
        description += (description.empty() ? "\n" : "\n\n");

        switch (ego)
        {
        case SPARM_RUNNING:
            if (item.sub_type == ARM_NAGA_BARDING)
                description += jtrans("It allows its wearer to slither at a great speed.");
            else
                description += jtrans("It allows its wearer to run at a great speed.");
            break;
        case SPARM_FIRE_RESISTANCE:
            description += jtrans("It protects its wearer from heat.");
            break;
        case SPARM_COLD_RESISTANCE:
            description += jtrans("It protects its wearer from cold.");
            break;
        case SPARM_POISON_RESISTANCE:
            description += jtrans("It protects its wearer from poison.");
            break;
        case SPARM_SEE_INVISIBLE:
            description += jtrans("It allows its wearer to see invisible things.");
            break;
        case SPARM_INVISIBILITY:
            description += jtrans("When activated it hides its wearer from "
                "the sight of others, but also increases "
                "their metabolic rate by a large amount.");
            break;
        case SPARM_STRENGTH:
            description += jtrans("It increases the physical power of its wearer (+3 to strength).");
            break;
        case SPARM_DEXTERITY:
            description += jtrans("It increases the dexterity of its wearer (+3 to dexterity).");
            break;
        case SPARM_INTELLIGENCE:
            description += jtrans("It makes you more clever (+3 to intelligence).");
            break;
        case SPARM_PONDEROUSNESS:
            description += jtrans("It is very cumbersome, thus slowing your movement.");
            break;
        case SPARM_FLYING:
            description += jtrans("It can be activated to allow its wearer to "
                "fly indefinitely.");
            break;
        case SPARM_MAGIC_RESISTANCE:
            description += jtrans("It increases its wearer's resistance "
                "to enchantments.");
            break;
        case SPARM_PROTECTION:
            description += jtrans("It protects its wearer from harm (+3 to AC).");
            break;
        case SPARM_STEALTH:
            description += jtrans("It enhances the stealth of its wearer.");
            break;
        case SPARM_RESISTANCE:
            description += jtrans("It protects its wearer from the effects "
                "of both cold and heat.");
            break;

        // These two are only for robes.
        case SPARM_POSITIVE_ENERGY:
            description += jtrans("It protects its wearer from "
                "the effects of negative energy.");
            break;
        case SPARM_ARCHMAGI:
            description += jtrans("It increases the power of its wearer's "
                "magical spells.");
            break;
#if TAG_MAJOR_VERSION == 34
        case SPARM_PRESERVATION:
            description += jtrans("It does nothing special.");
            break;
#endif
        case SPARM_REFLECTION:
            description += jtrans("It reflects blocked things back in the "
                "direction they came from.");
            break;

        case SPARM_SPIRIT_SHIELD:
            description += jtrans("It shields its wearer from harm at the cost "
                "of magical power.");
            break;

        case SPARM_NORMAL:
            ASSERT(enchanted);
            description += jtrans(
                           "It has no special ego (it is not resistant to "
                           "fire, etc), but is still enchanted in some way - "
                           "positive or negative.");

            break;

        // This is only for gloves.
        case SPARM_ARCHERY:
            description += jtrans("It improves your effectiveness with ranged weaponry (Slay+4).");
            break;
        }
    }

    if (is_artefact(item))
    {
        string rand_desc = _randart_descrip(item);
        if (!rand_desc.empty())
        {
            description += "\n";
            description += rand_desc;
        }

        // Can't happen, right? (XXX)
        if (!item_ident(item, ISFLAG_KNOW_PROPERTIES) && item_type_known(item))
            description += jtrans_notrim("\nThis armour may have some hidden properties.");
    }
    else
    {
        const int max_ench = armour_max_enchant(item);
        if (item.plus < max_ench || !item_ident(item, ISFLAG_KNOW_PLUSES))
        {
            description += make_stringf(jtrans_notrimc("\n\nIt can be maximally enchanted to +%s."),
                                        to_stringc(max_ench));
        }
        else
            description += jtrans_notrim("\n\nIt cannot be enchanted further.");
    }

    return sp2nbsp(description);
}

static string _describe_jewellery(const item_def &item, bool verbose)
{
    string description;
    string basename = jewellery_is_amulet(item) ? "amulet" : "ring";

    description.reserve(200);

    if (verbose && !is_artefact(item)
        && item_ident(item, ISFLAG_KNOW_PLUSES))
    {
        // Explicit description of ring power.
        if (item.plus != 0)
        {
            switch (item.sub_type)
            {
            case RING_PROTECTION:
                description += make_stringf(jtrans_notrimc("\nIt affects your AC (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            case RING_EVASION:
                description += make_stringf(jtrans_notrimc("\nIt affects your evasion (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            case RING_STRENGTH:
                description += make_stringf(jtrans_notrimc("\nIt affects your strength (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            case RING_INTELLIGENCE:
                description += make_stringf(jtrans_notrimc("\nIt affects your intelligence (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            case RING_DEXTERITY:
                description += make_stringf(jtrans_notrimc("\nIt affects your dexterity (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            case RING_SLAYING:
                description += make_stringf(jtrans_notrimc("\nIt affects your accuracy and"
                                                           " damage with ranged weapons and melee attacks (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            case AMU_REFLECTION:
                description += make_stringf(jtrans_notrimc("\nIt affects your shielding (%+d)."),
                                            jtransc(basename),
                                            item.plus);
                break;

            default:
                break;
            }
        }
    }

    // Artefact properties.
    if (is_artefact(item))
    {
        string rand_desc = _randart_descrip(item);
        if (!rand_desc.empty())
        {
            description += "\n";
            description += rand_desc;
        }
        if (!item_ident(item, ISFLAG_KNOW_PROPERTIES) ||
            !item_ident(item, ISFLAG_KNOW_TYPE))
        {
            description += make_stringf(jtrans_notrimc("\nThis {amulet|ring} may have hidden properties."),
                                        jtransc(basename));
        }
    }

    return description;
}

static bool _compare_card_names(card_type a, card_type b)
{
    return string(card_name(a)) < string(card_name(b));
}

static bool _check_buggy_deck(const item_def &deck, string &desc)
{
    if (!is_deck(deck))
    {
        desc += jtrans_notrim("This isn't a deck at all!\n");
        return true;
    }

    const CrawlHashTable &props = deck.props;

    if (!props.exists(CARD_KEY)
        || props[CARD_KEY].get_type() != SV_VEC
        || props[CARD_KEY].get_vector().get_type() != SV_BYTE
        || cards_in_deck(deck) == 0)
    {
        return true;
    }

    return false;
}

static string _describe_deck(const item_def &item)
{
    string description;

    description.reserve(100);

    description += "\n";

    if (_check_buggy_deck(item, description))
        return "";

    if (item_type_known(item))
        description += deck_contents(item.sub_type) + "\n";

    description += make_stringf(jtrans_notrimc("\nMost decks begin with %d to %d cards."),
                                MIN_STARTING_CARDS,
                                MAX_STARTING_CARDS);

    auto card_stringify = [](card_type card){ return card_name_j(card, false); };

    const vector<card_type> drawn_cards = get_drawn_cards(item);
    if (!drawn_cards.empty())
    {
        description += "\n";
        description += jtrans_notrim("Drawn card(s): ");
        description += comma_separated_fn(drawn_cards.begin(),
                                          drawn_cards.end(),
                                          card_stringify, "、", "、") + "のカード";
    }

    const int num_cards = cards_in_deck(item);
    // The list of known cards, ending at the first one not known to be at the
    // top.
    vector<card_type> seen_top_cards;
    // Seen cards in the deck not necessarily contiguous with the start. (If
    // Nemelex wrath shuffled a deck that you stacked, for example.)
    vector<card_type> other_seen_cards;
    bool still_contiguous = true;
    for (int i = 0; i < num_cards; ++i)
    {
        uint8_t flags;
        const card_type card = get_card_and_flags(item, -i-1, flags);
        if (flags & CFLAG_SEEN)
        {
            if (still_contiguous)
                seen_top_cards.push_back(card);
            else
                other_seen_cards.push_back(card);
        }
        else
            still_contiguous = false;
    }

    if (!seen_top_cards.empty())
    {
        description += "\n";
        description += jtrans_notrim("Next card(s): ");
        description += comma_separated_fn(seen_top_cards.begin(),
                                          seen_top_cards.end(),
                                          card_stringify, "、", "、") + "のカード";
    }
    if (!other_seen_cards.empty())
    {
        description += "\n";
        sort(other_seen_cards.begin(), other_seen_cards.end(),
             _compare_card_names);

        description += jtrans_notrim("Seen card(s): ");
        description += comma_separated_fn(other_seen_cards.begin(),
                                          other_seen_cards.end(),
                                          card_stringify, "、", "、") + "のカード";
    }

    return description;
}

bool is_dumpable_artefact(const item_def &item)
{
    return is_known_artefact(item) && item_ident(item, ISFLAG_KNOW_PROPERTIES);
}

/**
 * Describe a specified item.
 *
 * @param item    The specified item.
 * @param verbose Controls various switches for the length of the description.
 * @param dump    This controls which style the name is shown in.
 * @param lookup  If true, the name is not shown at all.
 *   If either of those two are true, the DB description is not shown.
 * @return a string with the name, db desc, and some other data.
 */

static string _spacer(const int length)
{
    return length < 0 ? "" : string(length, ' ');
}

string get_item_description(const item_def &item, bool verbose,
                            bool dump, bool lookup)
{
    ostringstream description;

    if (!dump && !lookup)
    {
        string name = item.name(DESC_INVENTORY_EQUIP);
        string name_en = (item.base_type == OBJ_BOOKS && is_artefact(item)) ? "" :
                         (is_artefact(item) && !is_unrandom_artefact(item)) ? "" :
                          is_artefact(item) ? uppercase_first(item.name_en(DESC_THE))
                                            : uppercase_first(item.name_en(DESC_A));
        string spacer = _spacer(get_number_of_cols() - strwidth(name)
                                                     - strwidth(name_en) - 1);

        const int min_spacer_size = 5;
        if (strwidth(name) + strwidth(name_en) + min_spacer_size > get_number_of_cols())
            name_en = "";

        string title = name + spacer + name_en;

        description << sp2nbsp(title);
    }

#ifdef DEBUG_DIAGNOSTICS
    if (!dump)
    {
        description << setfill('0');
        description << "\n"
                    << "base: " << static_cast<int>(item.base_type)
                    << " sub: " << static_cast<int>(item.sub_type)
                    << " plus: " << item.plus << " plus2: " << item.plus2
                    << " special: " << item.special
                    << "\n"
                    << "quant: " << item.quantity
                    << " rnd?: " << static_cast<int>(item.rnd)
                    << " flags: " << hex << setw(8) << item.flags
                    << dec << "\n"
                    << "x: " << item.pos.x << " y: " << item.pos.y
                    << " link: " << item.link
                    << " slot: " << item.slot
                    << " ident_type: "
                    << get_ident_type(item)
                    << "\nannotate: "
                    << stash_annotate_item(STASH_LUA_SEARCH_ANNOTATE, &item);
    }
#endif

    if (verbose || (item.base_type != OBJ_WEAPONS
                    && item.base_type != OBJ_ARMOUR
                    && item.base_type != OBJ_BOOKS))
    {
        description << "\n\n";

        bool need_base_desc = !lookup;

        if (dump)
        {
            description << "["
                        << item.name(DESC_DBNAME, true, false, false)
                        << "]";
            need_base_desc = false;
        }
        else if (is_unrandom_artefact(item) && item_type_known(item))
        {
            const string desc = getLongDescription(get_artefact_name(item));
            if (!desc.empty())
            {
                description << desc;
                need_base_desc = false;
                description.seekp((streamoff)-1, ios_base::cur);
                description << " ";
            }
        }
        // Randart jewellery properties will be listed later,
        // just describe artefact status here.
        else if (is_artefact(item) && item_type_known(item)
                 && item.base_type == OBJ_JEWELLERY)
        {
            description << jtrans("It is an ancient artefact.");
            need_base_desc = false;
        }

        if (need_base_desc)
        {
            string db_name_en = item.name_en(DESC_DBNAME, true, false, false);
            string db_desc = getLongDescription(db_name_en);

            if (db_desc.empty())
            {
                if (item_type_known(item))
                {
                    description << "[ERROR: no desc for item name '" << db_name_en
                                << "']. Perhaps this item has been removed?\n";
                }
                else
                {
                    description << item.name(DESC_A, true, false, false)
                                << "だ。\n";
                }
            }
            else
                description << db_desc << "\n";

            // Get rid of newline at end of description; in most cases we
            // will be adding "\n\n" immediately, and we want only one,
            // not two, blank lines. This allow allows the "unpleasant"
            // message for chunks to appear on the same line.
            description.seekp((streamoff)-1, ios_base::cur);
            description << " ";
        }
    }

    string desc;
    switch (item.base_type)
    {
    // Weapons, armour, jewellery, books might be artefacts.
    case OBJ_WEAPONS:
        desc += make_stringf(_describe_weapon(item, verbose).c_str(),
                             jtransc(base_type_string(item)));
        break;

    case OBJ_ARMOUR:
        desc += make_stringf(_describe_armour(item, verbose).c_str(),
                             jtransc(base_type_string(item)));
        break;

    case OBJ_JEWELLERY:
        desc += _describe_jewellery(item, verbose);
        break;

    case OBJ_BOOKS:
        if (!verbose
            && (Options.dump_book_spells || is_random_artefact(item)))
        {
            desc += describe_item_spells(item);
        }
        break;

    case OBJ_MISSILES:
        desc += _describe_ammo(item);
        break;

    case OBJ_WANDS:
    {
        const bool known_empty = is_known_empty_wand(item);

        if (!item_ident(item, ISFLAG_KNOW_PLUSES) && !known_empty)
        {
            desc += jtrans_notrim(
                           "\nIf evoked without being fully identified,"
                           " several charges will be wasted out of"
                           " unfamiliarity with the device.");
        }


        if (item_type_known(item) && !item_ident(item, ISFLAG_KNOW_PLUSES))
        {
            desc += make_stringf(jtrans_notrimc("\nIt can have at most %d charges."),
                                 wand_max_charges(item));
        }

        if (known_empty)
            desc += jtrans_notrim("\nUnfortunately, it has no charges left.");
        break;
    }

    case OBJ_CORPSES:
        if (item.sub_type == CORPSE_SKELETON)
            break;

        // intentional fall-through
    case OBJ_FOOD:
        if (item.base_type == OBJ_FOOD)
        {
            desc += "\n";

            const int turns = food_turns(item);
            ASSERT(turns > 0);
            if (turns > 1)
            {
                desc += jtrans(make_stringf("It is large enough that eating it takes %s"
                                            " turns, during which time the eater is vulnerable"
                                            " to attack.",
                                            ((turns > 2) ? "several" : "a couple of")));
            }
            else
                desc += jtrans("It is small enough that eating it takes "
                               "only one turn.");
        }
        if (item.base_type == OBJ_CORPSES || item.sub_type == FOOD_CHUNK)
        {
            switch (determine_chunk_effect(item))
            {
            case CE_MUTAGEN:
                desc += jtrans_notrim("\n\nEating this meat will cause random "
                               "mutations.");
                break;
            case CE_NOXIOUS:
                desc += jtrans_notrim("\n\nThis meat is toxic.");
                break;
            default:
                break;
            }
        }
        break;

    case OBJ_RODS:
        if (verbose)
        {
            desc += jtrans_notrim(
                "\nIt uses its own magic reservoir for casting spells, and "
                "recharges automatically according to the recharging "
                "rate.");

            const int max_charges = MAX_ROD_CHARGE;
            const int max_recharge_rate = MAX_WPN_ENCHANT;
            if (item_ident(item, ISFLAG_KNOW_PLUSES))
            {
                const int num_charges = item.charge_cap / ROD_CHARGE_MULT;
                if (max_charges > num_charges)
                {
                    desc += make_stringf(jtrans_notrimc("\nIt can currently hold {num}"
                                                        " charges. It can be magically "
                                                        "recharged to contain up to "
                                                        "{max} charges."),
                                         num_charges, max_charges);
                }
                else
                    desc += jtrans_notrim("\nIts capacity can be increased no further.");

                const int recharge_rate = item.rod_plus;
                if (recharge_rate < max_recharge_rate)
                {
                    desc += make_stringf(jtrans_notrimc("\nIts current recharge rate is {+rate}"
                                                        ". It can be magically recharged up to {+maxrate}."),
                                         recharge_rate, max_recharge_rate);
                }
                else
                    desc += jtrans_notrim("\nIts recharge rate is at maximum.");
            }
            else
            {
                desc += make_stringf(jtrans_notrimc("\nIt can have at most "
                                                    " charges and {+rate} recharge rate."),
                                     max_charges, max_recharge_rate);
            }
        }
        else if (Options.dump_book_spells)
        {
            desc += describe_item_spells(item);
        }

        break;

    case OBJ_STAVES:
        {
            string stats = "\n";
            _append_weapon_stats(stats, item);
            desc += stats;
        }
        desc += jtrans_notrim("\n\nIt falls into the 'Staves' category. ");
        desc += jtrans(_handedness_string(item));
        break;

    case OBJ_MISCELLANY:
        if (is_deck(item))
            desc += _describe_deck(item);
        if (is_xp_evoker(item))
        {
            desc += make_stringf(jtrans_notrimc("\n\nOnce activated, this device (and all other devices of its kind) "
                                                "will be rendered temporarily inert. However, (they|it) "
                                                "will recharge as you gain experience."),
                                 jtransc(!item_is_horn_of_geryon(item) ? "and all other devices of its kind " : ""),
                                 jtransc(!item_is_horn_of_geryon(item) ? "they " : "it "))
                  + jtrans_notrim(!evoker_is_charged(item) ? "\nThe device is presently inert." : "");
        }
        break;

    case OBJ_POTIONS:
#ifdef DEBUG_BLOOD_POTIONS
        // List content of timer vector for blood potions.
        if (!dump && is_blood_potion(item))
        {
            item_def stack = static_cast<item_def>(item);
            CrawlHashTable &props = stack.props;
            if (!props.exists("timer"))
                desc += "\nTimers not yet initialized.";
            else
            {
                CrawlVector &timer = props["timer"].get_vector();
                ASSERT(!timer.empty());

                desc += make_stringf("\nQuantity: %d        Timer size: %d",
                                     stack.quantity, (int)timer.size());
                desc += "\nTimers:\n";
                for (const CrawlStoreValue& store : timer)
                    desc += make_stringf("%d  ", store.get_int());
            }
        }
#endif

    case OBJ_SCROLLS:
    case OBJ_ORBS:
    case OBJ_GOLD:
    case OBJ_RUNES:
        // No extra processing needed for these item types.
        break;

    default:
        die("Bad item class");
    }
    if (!desc.empty())
        description << desc << (ends_with(desc, "\n") ? "" : "\n");

    if (!verbose && item_known_cursed(item))
        description << make_stringf(jtrans_notrimc("\nIt has a curse placed upon it."),
                                    jtransc(base_type_string(item)));
    else
    {
        if (verbose)
        {
            if (item_known_cursed(item))
                description << make_stringf(jtrans_notrimc("\nIt has a curse placed upon it."),
                                            jtransc(base_type_string(item)));

            if (is_artefact(item))
            {
                if (item.base_type == OBJ_ARMOUR
                    || item.base_type == OBJ_WEAPONS)
                {
                    description << jtrans_notrim("\nThis ancient artefact cannot be changed "
                        "by magic or mundane means.");
                }
                // Randart jewellery has already displayed this line.
                else if (item.base_type != OBJ_JEWELLERY
                         || (item_type_known(item) && is_unrandom_artefact(item)))
                {
                    description << jtrans_notrim("\nIt is an ancient artefact.");
                }
            }
        }
    }

    if (god_hates_item_handling(item))
    {
        description << "\n\n" << god_name_j(you.religion)
                    << jtrans(" disapproves of the use of such an item.");
    }

    if (verbose && origin_describable(item))
        description << "\n" << origin_desc(item, true);

    // This information is obscure and differs per-item, so looking it up in
    // a docs file you don't know to exist is tedious.

    string prefix_guide;
    if (verbose)
    {
        prefix_guide += jtrans_notrim("\nStash search prefixes: ")
                     + userdef_annotate_item(STASH_LUA_SEARCH_ANNOTATE, &item);
        string menu_prefix = item_prefix(item, false);
        if (!menu_prefix.empty())
            prefix_guide += jtrans_notrim("\nMenu/colouring prefixes: ") + menu_prefix;
    }

    string description_text = description.str();
    if (!prefix_guide.empty())
    {
        description_text += (ends_with(description_text, "\n") ? "" : "\n")
                          + prefix_guide;
    }

    return description_text;
}

void get_feature_desc(const coord_def &pos, describe_info &inf)
{
    dungeon_feature_type feat = env.map_knowledge(pos).feat();

    string desc      = feature_description_at(pos, false, DESC_A, false);
    string desc_en   = feature_description_at_en(pos, false, DESC_A, false);
    string db_name   = feat == DNGN_ENTER_SHOP ? "a shop" : desc_en;
    string long_desc = getLongDescription(db_name);
    string spacer    = _spacer(get_number_of_cols() - strwidth(desc)
                                                    - strwidth(desc_en) - 1);

    string title = desc + spacer + (desc != desc_en ? desc_en : "");

    // e.g. "a wall of the weird stuff which makes up Pandemonium"
    const int min_spacer_size = 5;
    if ((strwidth(desc) + strwidth(desc_en) + min_spacer_size) > get_number_of_cols())
        inf.title = desc;
    else
        inf.title = sp2nbsp(title);

    const string marker_desc =
        env.markers.property_at(pos, MAT_ANY, "feature_description_long");

    // suppress this if the feature changed out of view
    if (!marker_desc.empty() && grd(pos) == feat)
        long_desc += sp2nbsp(marker_desc);

    // Display branch descriptions on the entries to those branches.
    if (feat_is_stair(feat))
    {
        for (branch_iterator it; it; ++it)
        {
            if (it->entry_stairs == feat)
            {
                long_desc += "\n";
                long_desc += getLongDescription(it->shortname);
                break;
            }
        }
    }

    // mention the ability to pray at altars
    if (feat_is_altar(feat))
    {
        long_desc +=
            make_stringf(jtrans_notrimc("\n(Pray here with '%s' to learn more.)\n"),
                         command_to_string(CMD_GO_DOWNSTAIRS).c_str());
    }

    inf.body << long_desc;

    if (!feat_is_solid(feat))
    {
        string area_desc = "";
        if (haloed(pos) && !umbraed(pos))
            area_desc += "\n" + getLongDescription("haloed");
        if (umbraed(pos) && !haloed(pos))
            area_desc += "\n" + getLongDescription("umbraed");
        if (liquefied(pos))
            area_desc += "\n" + getLongDescription("liquefied");
        if (disjunction_haloed(pos))
            area_desc += "\n" + getLongDescription("disjunction haloed");

        inf.body << area_desc;
    }

    if (const cloud_type cloud = env.map_knowledge(pos).cloud())
    {
        const string cl_name = cloud_type_name_j(cloud);
        const string cl_name_en = cloud_type_name(cloud);
        const string cl_desc = getLongDescription(cl_name_en + " cloud");
        inf.body << make_stringf(jtrans_notrimc("\n\nA cloud of {cloud}."),
                                 cl_name.c_str())
                 << (cl_desc.empty() ? "" : "\n\n")
                 << cl_desc << extra_cloud_info(cloud);
    }

    inf.quote = getQuoteString(db_name);
}

/// A message explaining how the player can toggle between quote &
static const string _toggle_message =
    "Press '<w>!</w>'"
#ifdef USE_TILE_LOCAL
    " or <w>Right-click</w>"
#endif
    " to toggle between the description and quote.";

/**
 * If the given description has an associated quote, print a message at the
 * bottom of the screen explaining how the player can toggle between viewing
 * that quote & the description, and then check whether the input corresponds
 * to such a toggle.
 *
 * @param inf[in]       The description in question.
 * @param key[in,out]   The input command. If zero, is set to getchm().
 * @return              Whether the description & quote should be toggled.
 */
static int _print_toggle_message(const describe_info &inf, int& key)
{
    mouse_control mc(MOUSE_MODE_MORE);

    if (inf.quote.empty())
    {
        if (!key)
            key = getchm();
        return false;
    }

    const int bottom_line = min(30, get_number_of_lines());
    cgotoxy(1, bottom_line);
    formatted_string::parse_string(jtrans(_toggle_message)).display();
    if (!key)
        key = getchm();

    if (key == '!' || key == CK_MOUSE_CMD)
        return true;

    return false;
}

void describe_feature_wide(const coord_def& pos, bool show_quote)
{
    describe_info inf;
    get_feature_desc(pos, inf);

#ifdef USE_TILE_WEB
    tiles_crt_control show_as_menu(CRT_MENU, "describe_feature");
#endif

    if (show_quote)
        _print_quote(inf);
    else
        print_description(inf);

    if (crawl_state.game_is_hints())
        hints_describe_pos(pos.x, pos.y);

    int key = 0;
    if (_print_toggle_message(inf, key))
        describe_feature_wide(pos, !show_quote);
}

void get_item_desc(const item_def &item, describe_info &inf)
{
    // Don't use verbose descriptions if the item contains spells,
    // so we can actually output these spells if space is scarce.
    const bool verbose = !item.has_spells();
    inf.body << get_item_description(item, verbose);
}

static vector<command_type> _allowed_actions(const item_def& item)
{
    vector<command_type> actions;
    actions.push_back(CMD_ADJUST_INVENTORY);
    if (item_equip_slot(item) == EQ_WEAPON)
        actions.push_back(CMD_UNWIELD_WEAPON);
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
    case OBJ_STAVES:
    case OBJ_RODS:
    case OBJ_MISCELLANY:
        if (!item_is_equipped(item))
        {
            if (item_is_wieldable(item))
                actions.push_back(CMD_WIELD_WEAPON);
            if (is_throwable(&you, item))
                actions.push_back(CMD_QUIVER_ITEM);
        }
        break;
    case OBJ_MISSILES:
        if (you.species != SP_FELID)
            actions.push_back(CMD_QUIVER_ITEM);
        break;
    case OBJ_ARMOUR:
        if (item_is_equipped(item))
            actions.push_back(CMD_REMOVE_ARMOUR);
        else
            actions.push_back(CMD_WEAR_ARMOUR);
        break;
    case OBJ_FOOD:
        if (can_eat(item, true, false))
            actions.push_back(CMD_EAT);
        break;
    case OBJ_SCROLLS:
    //case OBJ_BOOKS: these are handled differently
        actions.push_back(CMD_READ);
        break;
    case OBJ_JEWELLERY:
        if (item_is_equipped(item))
            actions.push_back(CMD_REMOVE_JEWELLERY);
        else
            actions.push_back(CMD_WEAR_JEWELLERY);
        break;
    case OBJ_POTIONS:
        if (!you_foodless(true)) // mummies and lich form forbidden
            actions.push_back(CMD_QUAFF);
        break;
    default:
        ;
    }
#if defined(CLUA_BINDINGS)
    if (clua.callbooleanfn(false, "ch_item_wieldable", "i", &item))
        actions.push_back(CMD_WIELD_WEAPON);
#endif

    if (item_is_evokable(item))
        actions.push_back(CMD_EVOKE);

    actions.push_back(CMD_DROP);

    if (!crawl_state.game_is_tutorial())
        actions.push_back(CMD_INSCRIBE_ITEM);

    return actions;
}

static string _actions_desc(const vector<command_type>& actions, const item_def& item)
{
    static const map<command_type, string> act_str =
    {
        { CMD_WIELD_WEAPON, "(w)ield" },
        { CMD_UNWIELD_WEAPON, "(u)nwield" },
        { CMD_QUIVER_ITEM, "(q)uiver" },
        { CMD_WEAR_ARMOUR, "(w)ear" },
        { CMD_REMOVE_ARMOUR, "(t)ake off" },
        { CMD_EVOKE, "e(v)oke" },
        { CMD_EAT, "(e)at" },
        { CMD_READ, "(r)ead" },
        { CMD_WEAR_JEWELLERY, "(p)ut on" },
        { CMD_REMOVE_JEWELLERY, "(r)emove" },
        { CMD_QUAFF, "(q)uaff" },
        { CMD_DROP, "(d)rop" },
        { CMD_INSCRIBE_ITEM, "(i)nscribe" },
        { CMD_ADJUST_INVENTORY, "(=)adjust" },
    };
    return make_stringf(jtransc("You can {cmd list} the {name}."),
                        jtransc(item.name(DESC_BASENAME)),
                        comma_separated_fn(begin(actions), end(actions),
                                [] (command_type cmd)
                                {
                                    return jtrans(act_str.at(cmd));
                                },
                                "/", "/").c_str());
}

// Take a key and a list of commands and return the command from the list
// that corresponds to the key. Note that some keys are overloaded (but with
// mutually-exclusive actions), so it's not just a simple lookup.
static command_type _get_action(int key, vector<command_type> actions)
{
    static const map<command_type, int> act_key =
    {
        { CMD_WIELD_WEAPON,     'w' },
        { CMD_UNWIELD_WEAPON,   'u' },
        { CMD_QUIVER_ITEM,      'q' },
        { CMD_WEAR_ARMOUR,      'w' },
        { CMD_REMOVE_ARMOUR,    't' },
        { CMD_EVOKE,            'v' },
        { CMD_EAT,              'e' },
        { CMD_READ,             'r' },
        { CMD_WEAR_JEWELLERY,   'p' },
        { CMD_REMOVE_JEWELLERY, 'r' },
        { CMD_QUAFF,            'q' },
        { CMD_DROP,             'd' },
        { CMD_INSCRIBE_ITEM,    'i' },
        { CMD_ADJUST_INVENTORY, '=' },
    };

    key = tolower(key);

    for (auto cmd : actions)
        if (key == act_key.at(cmd))
            return cmd;

    return CMD_NO_CMD;
}

/**
 * Do the specified action on the specified item.
 *
 * @param item    the item to have actions done on
 * @param actions the list of actions to search in
 * @param keyin   the key that was pressed
 * @return whether to stay in the inventory menu afterwards
 */
static bool _do_action(item_def &item, const vector<command_type>& actions, int keyin)
{
    const command_type action = _get_action(keyin, actions);
    if (action == CMD_NO_CMD)
        return true;

    const int slot = item.link;
    ASSERT_RANGE(slot, 0, ENDOFPACK);

    redraw_screen();
    switch (action)
    {
    case CMD_WIELD_WEAPON:     wield_weapon(true, slot);            break;
    case CMD_UNWIELD_WEAPON:   wield_weapon(true, SLOT_BARE_HANDS); break;
    case CMD_QUIVER_ITEM:      quiver_item(slot);                   break;
    case CMD_WEAR_ARMOUR:      wear_armour(slot);                   break;
    case CMD_REMOVE_ARMOUR:    takeoff_armour(slot);                break;
    case CMD_EVOKE:            evoke_item(slot);                    break;
    case CMD_EAT:              eat_food(slot);                      break;
    case CMD_READ:             read(&item);                         break;
    case CMD_WEAR_JEWELLERY:   puton_ring(slot);                    break;
    case CMD_REMOVE_JEWELLERY: remove_ring(slot, true);             break;
    case CMD_QUAFF:            drink(&item);                        break;
    case CMD_DROP:             drop_item(slot, item.quantity);      break;
    case CMD_INSCRIBE_ITEM:    inscribe_item(item);                 break;
    case CMD_ADJUST_INVENTORY: adjust_item(slot);                   break;
    default:
        die("illegal inventory cmd %d", action);
    }
    return false;
}

/**
 *  Describe any item in the game.
 *
 *  @param item       the item to be described.
 *  @param fixup_desc a function (possibly null) to modify the
 *                    description before it's displayed.
 *  @return whether to stay in the inventory menu afterwards.
 */
bool describe_item(item_def &item, function<void (string&)> fixup_desc)
{
    if (!item.defined())
        return true;

#ifdef USE_TILE_WEB
    tiles_crt_control show_as_menu(CRT_MENU, "describe_item");
#endif

    string desc = get_item_description(item, true, false);

    string quote;
    if (is_unrandom_artefact(item) && item_type_known(item))
        quote = getQuoteString(get_artefact_name(item));
    else
        quote = getQuoteString(item.name_en(DESC_DBNAME, true, false, false));

    if (!(crawl_state.game_is_hints_tutorial()
          || quote.empty()))
    {
        desc += "\n\n" + quote;
    }

    if (crawl_state.game_is_hints())
        desc += "\n\n" + hints_describe_item(item);

    if (fixup_desc)
        fixup_desc(desc);
    // spellbooks & rods have their own UIs, so we don't currently support the
    // inscribe/drop/etc prompt UI for them.
    // ...it would be nice if we did, though.
    if (item.has_spells())
    {
        formatted_string fdesc(formatted_string::parse_string(desc));
        list_spellset(item_spellset(item), nullptr, &item, fdesc);
        // only continue the inventory loop if we didn't start memorizing a
        // spell & didn't destroy the item for amnesia.
        return !already_learning_spell() && item.is_valid();
    }
    else
    {
        const bool do_actions = in_inventory(item) // Dead men use no items.
                                && !(you.pending_revival
                                     || crawl_state.updating_scores);
        vector<command_type> actions;
        formatted_scroller menu;
        menu.add_text(desc, false, get_number_of_cols());
        if (do_actions)
        {
            actions = _allowed_actions(item);
            menu.set_flags(menu.get_flags() | MF_ALWAYS_SHOW_MORE);
            menu.set_more(formatted_string(_actions_desc(actions, item), CYAN));
        }
        menu.show();
        if (do_actions)
            return _do_action(item, actions, menu.getkey());
        else
            return true;
    }
}

void inscribe_item(item_def &item)
{
    mprf_nocap(MSGCH_EQUIPMENT, "%s", item.name(DESC_INVENTORY).c_str());

    const bool is_inscribed = !item.inscription.empty();
    string prompt = is_inscribed ? "Replace inscription with what? "
                                 : "Inscribe with what? ";

    char buf[79];
    int ret = msgwin_get_line(prompt, buf, sizeof buf, nullptr,
                              item.inscription);
    if (ret)
    {
        canned_msg(MSG_OK);
        return;
    }

    string new_inscrip = buf;
    trim_string_right(new_inscrip);

    if (item.inscription == new_inscrip)
    {
        canned_msg(MSG_OK);
        return;
    }

    item.inscription = new_inscrip;

    mprf_nocap(MSGCH_EQUIPMENT, "%s", item.name(DESC_INVENTORY).c_str());
    you.wield_change  = true;
    you.redraw_quiver = true;
}

/**
 * List the simple calculated stats of a given spell, when cast by the player
 * in their current condition.
 *
 * @param spell     The spell in question.
 * @param rod       Whether the spell is being cast from a rod (not a book).
 */
static string _player_spell_stats(const spell_type spell, bool rod)
{
    string description;
    description += make_stringf(jtrans_notrimc("\nLevel: %d"), spell_difficulty(spell));
    if (!rod)
    {
        const string schools = spell_schools_string(spell);
        description +=
            make_stringf(jtrans_notrimc("        School%s: %s"),
                         schools.c_str());

        if (!crawl_state.need_save
            || (get_spell_flags(spell) & SPFLAG_MONSTER))
        {
            return description; // all other info is player-dependent
        }

        const string failure = failure_rate_to_string(raw_spell_fail(spell));
        description += make_stringf(jtrans_notrimc("        Fail: %s"), failure.c_str());
    }

    description += jtrans_notrim("\n\nPower : ");
    description += spell_power_string(spell, rod);
    description += jtrans_notrim("\nRange : ");
    description += spell_range_string(spell, rod);
    description += jtrans_notrim("\nHunger: ");
    description += spell_hunger_string(spell, rod);
    description += jtrans_notrim("\nNoise : ");
    description += spell_noise_string(spell);
    return sp2nbsp(description);
}

string get_skill_description(skill_type skill, bool need_title)
{
    string lookup = skill_name_j(skill);
    string lookup_en = skill_name(skill);
    string result = "";
    string spacer = _spacer(get_number_of_cols() - strwidth(lookup)
                                                 - strwidth(lookup_en) - 1);

    if (need_title)
    {
        result = lookup + spacer + (lookup != lookup_en ? lookup_en : "");
        result += "\n\n";
    }

    result += getLongDescription(lookup_en);

    switch (skill)
    {
        case SK_INVOCATIONS:
            if (you.species == SP_DEMIGOD)
            {
                result += "\n";
                result += jtrans("How on earth did you manage to pick this up?");
            }
            else if (you_worship(GOD_TROG))
            {
                result += "\n";
                result += jtrans(
                          "Note that Trog doesn't use Invocations, due to its "
                          "close connection to magic.");
            }
            break;

        case SK_SPELLCASTING:
            if (you_worship(GOD_TROG))
            {
                result += "\n";
                result += jtrans(
                          "Keep in mind, though, that Trog will greatly "
                          "disapprove of this.");
            }
            break;
        default:
            // No further information.
            break;
    }

    return sp2nbsp(result);
}

/// How much power do we think the given monster casts this spell with?
static int _hex_pow(const spell_type spell, const int hd)
{
    const int cap = 200;
    const int pow = mons_power_for_hd(spell, hd, false) / ENCH_POW_FACTOR;
    return min(cap, pow);
}

/**
 * What are the odds of the given spell, cast by a monster with the given
 * spell_hd, affecting the player?
 */
int hex_chance(const spell_type spell, const int hd)
{
    const int capped_pow = _hex_pow(spell, hd);
    const int chance = hex_success_chance(you.res_magic(), capped_pow,
                                          100, true);
    if (spell == SPELL_STRIP_RESISTANCE)
        return chance + (100 - chance) / 3; // ignores mr 1/3rd of the time
    return chance;
}

/**
 * Describe mostly non-numeric player-specific information about a spell.
 *
 * (E.g., your god's opinion of it, whether it's in a high-level book that
 * you can't memorise from, whether it's currently useless for whatever
 * reason...)
 *
 * @param spell     The spell in question.
 * @param item      The object the spell is in; may be null.
 */
static string _player_spell_desc(spell_type spell, const item_def* item)
{
    if (!crawl_state.need_save || (get_spell_flags(spell) & SPFLAG_MONSTER))
        return ""; // all info is player-dependent

    string description;

    // Report summon cap
    const int limit = summons_limit(spell);
    if (limit)
    {
        description += make_stringf(jtrans_notrimc("\nYou can sustain at most {num}"
                                                   " creature(s) summoned by this spell."),
                                    to_stringc(limit));
    }

    const bool rod = item && item->base_type == OBJ_RODS;
    if (god_hates_spell(spell, you.religion, rod))
    {
        description += "\n" + god_name_j(you.religion)
                     + jtrans(" frowns upon the use of this spell.");
        if (god_loathes_spell(spell, you.religion))
            description += jtrans_notrim("You'd be excommunicated if you dared to cast it!");
    }
    else if (god_likes_spell(spell, you.religion))
    {
        description += "\n" + god_name_j(you.religion)
                     + jtransln(" supports the use of this spell.");
    }

    if (!you_can_memorise(spell))
    {
        string reason = jtrans(desc_cannot_memorise_reason(spell));
        reason = replace_all(reason, "あなたは", "");
        reason = replace_all(reason, "あなたが", "");
        reason = replace_all(reason, "あなたの魔力は", "魔力が");
        reason = replace_all(reason, "あなたの", "");
        reason = replace_all(reason, "だ。", "なため");
        reason = replace_all(reason, "。", "ため");

        description += make_stringf(jtrans_notrimc("\nYou cannot memorise this spell because {reason}"),
                                    reason.c_str());
    }
    else if (spell_is_useless(spell, true, false, rod))
    {
        string reason = jtrans(spell_uselessness_reason(spell, true, false, rod));
        reason = replace_all(reason, "あなたは", "");
        reason = replace_all(reason, "あなたが", "");
        reason = replace_all(reason, "あなたの魔力は", "魔力が");
        reason = replace_all(reason, "あなたの", "");
        reason = replace_all(reason, "だ。", "なため");
        reason = replace_all(reason, "。", "ため");

        description += make_stringf(jtrans_notrimc("\nThis spell will have no effect right now because {reason}"),
                                    reason.c_str());
    }

    if(!description.empty())
        description = "\n" + description;

    return sp2nbsp(description);
}


/**
 * Describe a spell, as cast by the player.
 *
 * @param spell     The spell in question.
 * @param item      The object the spell is in; may be null.
 * @return          Information about the spell; does not include the title or
 *                  db description, but does include level, range, etc.
 */
string player_spell_desc(spell_type spell, const item_def* item)
{
    const bool rod = item && item->base_type == OBJ_RODS;
    return _player_spell_stats(spell, rod)
           + _player_spell_desc(spell, item);
}

/**
 * Examine a given spell. Set the given string to its description, stats, &c.
 * If it's a book in a spell that the player is holding, mention the option to
 * memorise it.
 *
 * @param spell         The spell in question.
 * @param mon_owner     If this spell is being examined from a monster's
 *                      description, 'spell' is that monster. Else, null.
 * @param description   Set to the description & details of the spell.
 * @param item          The item (book or rod) holding the spell, if any.
 * @return              Whether you can memorise the spell.
 */
static bool _get_spell_description(const spell_type spell,
                                  const monster_info *mon_owner,
                                  string &description,
                                  const item_def* item = nullptr)
{
    description.reserve(500);

    string title_ja = spell_title_j(spell);
    string title_en = spell_title(spell);
    string spacer = _spacer(get_number_of_cols() - strwidth(title_ja)
                                                 - strwidth(title_en) - 1);
    description = title_ja + spacer + title_en;
    description = sp2nbsp(description);
    description += "\n\n";
    const string long_descrip = getLongDescription(string(spell_title(spell))
                                                   + " spell");

    if (!long_descrip.empty())
        description += long_descrip;
    else
    {
        description += jtrans(
                       "This spell has no description. "
                       "Casting it may therefore be unwise. "
#ifdef DEBUG
                       "Instead, go fix it. ");
#else
                       "Please file a bug report.");
#endif
    }

    if (mon_owner)
    {
        const int hd = mon_owner->spell_hd();
        const int range = mons_spell_range(spell, hd);
        description += jtrans_notrim("\nRange : ")
                       + range_string(range, range, mons_char(mon_owner->type))
                       + "\n";

        // only display this if the player exists (not in the main menu)
        if (crawl_state.need_save && (get_spell_flags(spell) & SPFLAG_MR_CHECK)
#ifndef DEBUG_DIAGNOSTICS
            && mon_owner->attitude != ATT_FRIENDLY
#endif
            )
        {
            string wiz_info;
#ifdef WIZARD
            if (you.wizard)
                wiz_info += make_stringf(jtrans_notrimc(" (pow %d)"), _hex_pow(spell, hd));
#endif
            description += make_stringf(jtrans_notrimc("Chance to beat your MR: %d%%%s\n"),
                                        hex_chance(spell, hd),
                                        wiz_info.c_str());
        }

    }
    else
        description += player_spell_desc(spell, item);

    // Don't allow memorization after death.
    // (In the post-game inventory screen.)
    if (crawl_state.player_is_dead())
        return false;

    const string quote = getQuoteString(string(spell_title(spell)) + " spell");
    if (!quote.empty())
        description += "\n" + quote;

    if (item && item->base_type == OBJ_BOOKS && in_inventory(*item)
        && !you.has_spell(spell) && you_can_memorise(spell))
    {
        description += (ends_with(description, "\n") ? "" : "\n")
                     + jtrans_notrim("\n(M)emorise this spell.\n");
        return true;
    }

    return false;
}

/**
 * Provide the text description of a given spell.
 *
 * @param spell     The spell in question.
 * @param inf[out]  The spell's description is concatenated onto the end of
 *                  inf.body.
 */
void get_spell_desc(const spell_type spell, describe_info &inf)
{
    string desc;
    _get_spell_description(spell, nullptr, desc);
    inf.body << sp2nbsp(desc);
}


/**
 * Examine a given spell. List its description and details, and handle
 * memorizing the spell in question, if the player is able & chooses to do so.
 *
 * @param spelled   The spell in question.
 * @param mon_owner If this spell is being examined from a monster's
 *                  description, 'mon_owner' is that monster. Else, null.
 * @param item      The item (book or rod) holding the spell, if any.
 */
void describe_spell(spell_type spelled, const monster_info *mon_owner,
                    const item_def* item)
{
#ifdef USE_TILE_WEB
    tiles_crt_control show_as_menu(CRT_MENU, "describe_spell");
#endif

    string desc;
    const bool can_mem = _get_spell_description(spelled, mon_owner, desc, item);
    print_description(desc);

    mouse_control mc(MOUSE_MODE_MORE);
    char ch;
    if ((ch = getchm()) == 0)
        ch = getchm();

    if (can_mem && toupper(ch) == 'M')
    {
        redraw_screen();
        if (!learn_spell(spelled) || !you.turn_is_over)
            more();
        redraw_screen();
    }
}

static string _describe_draconian(const monster_info& mi)
{
    string description, scale_colour;
    const int subsp = mi.draco_or_demonspawn_subspecies();

    if (subsp != mi.type)
    {
        switch (subsp)
        {
        case MONS_BLACK_DRACONIAN:      scale_colour += "black ";   break;
        case MONS_MOTTLED_DRACONIAN:    scale_colour += "mottled "; break;
        case MONS_YELLOW_DRACONIAN:     scale_colour += "yellow ";  break;
        case MONS_GREEN_DRACONIAN:      scale_colour += "green ";   break;
        case MONS_PURPLE_DRACONIAN:     scale_colour += "purple ";  break;
        case MONS_RED_DRACONIAN:        scale_colour += "red ";     break;
        case MONS_WHITE_DRACONIAN:      scale_colour += "white ";   break;
        case MONS_GREY_DRACONIAN:       scale_colour += "grey ";    break;
        case MONS_PALE_DRACONIAN:       scale_colour += "pale ";    break;
        default:
            break;
        }

        description += make_stringf(jtranslnc("It has {color} scales. "),
                                    tagged_jtransc("[scale color]", scale_colour));
    }

    switch (subsp)
    {
    case MONS_BLACK_DRACONIAN:
        description += jtrans("Sparks flare out of its mouth and nostrils.");
        break;
    case MONS_MOTTLED_DRACONIAN:
        description += jtrans("Liquid flames drip from its mouth.");
        break;
    case MONS_YELLOW_DRACONIAN:
        description += jtrans("Acidic fumes swirl around it.");
        break;
    case MONS_GREEN_DRACONIAN:
        description += jtrans("Venom drips from its jaws.");
        break;
    case MONS_PURPLE_DRACONIAN:
        description += jtrans("Its outline shimmers with magical energy.");
        break;
    case MONS_RED_DRACONIAN:
        description += jtrans("Smoke pours from its nostrils.");
        break;
    case MONS_WHITE_DRACONIAN:
        description += jtrans("Frost pours from its nostrils.");
        break;
    case MONS_GREY_DRACONIAN:
        description += jtrans("Its scales and tail are adapted to the water.");
        break;
    case MONS_PALE_DRACONIAN:
        description += jtrans("It is cloaked in a pall of superheated steam.");
        break;
    default:
        break;
    }

    return description;
}

static string _describe_demonspawn_role(monster_type type)
{
    switch (type)
    {
    case MONS_BLOOD_SAINT:
        return "It weaves powerful and unpredictable spells of devastation.";
    case MONS_WARMONGER:
        return "It is devoted to combat, disrupting the magic of its foes as "
               "it battles endlessly.";
    case MONS_CORRUPTER:
        return "It corrupts space around itself, and can twist even the very "
               "flesh of its opponents.";
    case MONS_BLACK_SUN:
        return "It shines with an unholy radiance, and wields powers of "
               "darkness from its devotion to the deities of death.";
    default:
        return "";
    }
}

static string _describe_demonspawn_base(int species)
{
    switch (species)
    {
    case MONS_MONSTROUS_DEMONSPAWN:
        return "It is more beast now than whatever species it is descended from.";
    case MONS_GELID_DEMONSPAWN:
        return "It is covered in icy armour.";
    case MONS_INFERNAL_DEMONSPAWN:
        return "It gives off an intense heat.";
    case MONS_TORTUROUS_DEMONSPAWN:
        return "It menaces with bony spines.";
    }
    return "";
}

static string _describe_demonspawn(const monster_info& mi)
{
    string description;
    const int subsp = mi.draco_or_demonspawn_subspecies();

    description += jtrans(_describe_demonspawn_base(subsp));

    if (subsp != mi.type)
    {
        const string demonspawn_role = jtrans(_describe_demonspawn_role(mi.type));
        if (!demonspawn_role.empty())
            description += "\n" + demonspawn_role;
    }

    return description;
}

static const char* _get_resist_name(mon_resist_flags res_type)
{
    switch (res_type)
    {
    case MR_RES_ELEC:
        return "electricity";
    case MR_RES_POISON:
        return "poison";
    case MR_RES_FIRE:
        return "fire";
    case MR_RES_STEAM:
        return "steam";
    case MR_RES_COLD:
        return "cold";
    case MR_RES_ACID:
        return "acid";
    case MR_RES_ROTTING:
        return "rotting";
    case MR_RES_NEG:
        return "negative energy";
    case MR_RES_DAMNATION:
        return "damnation";
    default:
        return "buggy resistance";
    }
}

static const char* _get_threat_desc(mon_threat_level_type threat)
{
    switch (threat)
    {
    case MTHRT_TRIVIAL: return "harmless";
    case MTHRT_EASY:    return "easy";
    case MTHRT_TOUGH:   return "dangerous";
    case MTHRT_NASTY:   return "extremely dangerous";
    case MTHRT_UNDEF:
    default:            return "buggily threatening";
    }
}

static const char* _describe_attack_flavour(attack_flavour flavour)
{
    switch (flavour)
    {
    case AF_ACID:            return "deal extra acid damage";
    case AF_BLINK:           return "blink self";
    case AF_COLD:            return "deal extra cold damage";
    case AF_CONFUSE:         return "cause confusion";
    case AF_DRAIN_STR:       return "drain strength";
    case AF_DRAIN_INT:       return "drain intelligence";
    case AF_DRAIN_DEX:       return "drain dexterity";
    case AF_DRAIN_STAT:      return "drain strength, intelligence or dexterity";
    case AF_DRAIN_XP:        return "drain skills";
    case AF_ELEC:            return "cause electrocution";
    case AF_FIRE:            return "deal extra fire damage";
    case AF_HUNGER:          return "cause hungering";
    case AF_MUTATE:          return "cause mutations";
    case AF_POISON_PARALYSE: return "poison and cause paralysis or slowing";
    case AF_POISON:          return "cause poisoning";
    case AF_POISON_STRONG:   return "cause strong poisoning";
    case AF_ROT:             return "cause rotting";
    case AF_VAMPIRIC:        return "drain health from the living";
    case AF_KLOWN:           return "cause random powerful effects";
    case AF_DISTORT:         return "cause wild translocation effects";
    case AF_RAGE:            return "cause berserking";
    case AF_STICKY_FLAME:    return "apply sticky flame";
    case AF_CHAOTIC:         return "cause unpredictable effects";
    case AF_STEAL:           return "steal items";
    case AF_CRUSH:           return "constrict";
    case AF_REACH:           return "deal damage from a distance";
    case AF_HOLY:            return "deal extra damage to undead and demons";
    case AF_ANTIMAGIC:       return "drain magic";
    case AF_PAIN:            return "cause pain to the living";
    case AF_ENSNARE:         return "ensnare with webbing";
    case AF_ENGULF:          return "engulf with water";
    case AF_PURE_FIRE:       return "deal pure fire damage";
    case AF_DRAIN_SPEED:     return "drain speed";
    case AF_VULN:            return "reduce resistance to hostile enchantments";
    case AF_SHADOWSTAB:      return "deal extra damage from the shadows";
    case AF_DROWN:           return "deal drowning damage";
    case AF_CORRODE:         return "cause corrosion";
    case AF_SCARAB:          return "drain speed and drain health";
    case AF_TRAMPLE:         return "knock back the defender";
    case AF_REACH_STING:     return "cause poisoning from a distance";
    case AF_WEAKNESS:        return "cause weakness";
    case AF_KITE:            return "retreat from adjacent targets";
    case AF_SWOOP:           return "swoop behind its target";
    default:                 return "";
    }
}

static string _monster_attacks_description(const monster_info& mi)
{
    ostringstream result;
    set<attack_flavour> attack_flavours;
    vector<string> attack_descs;

    for (const auto &attack : mi.attack)
    {
        attack_flavour af = attack.flavour;
        if (!attack_flavours.count(af))
        {
            attack_flavours.insert(af);
            const char * const desc = _describe_attack_flavour(af);
            if (desc[0]) // non-empty
                attack_descs.push_back(jtrans(desc));
        }
    }

    if (!attack_descs.empty())
    {
        string pronoun = mi.pronoun_j(PRONOUN_SUBJECTIVE);
        if (pronoun == jtrans("It"))
            pronoun = "このモンスター";

        string term_text = "ことがある。\n";
        string term_text_with_pronoun = term_text + pronoun + "は";

        result << make_stringf(jtrans_notrimc("%s may attack to {attack flavours}.\n"),
                               pronoun.c_str(),
                               to_separated_line(attack_descs.begin(), attack_descs.end(),
                                                 term_text_with_pronoun,
                                                 term_text_with_pronoun).c_str());
    }

    return result.str();
}

static string _monster_spells_description(const monster_info& mi)
{
    static const string panlord_desc =
        jtrans_notrim("It may possess any of a vast number of diabolical powers.\n");

    // Show a generic message for pan lords, since they're secret.
    if (mi.type == MONS_PANDEMONIUM_LORD && !mi.props.exists(SEEN_SPELLS_KEY))
        return panlord_desc;

    // Show monster spells and spell-like abilities.
    if (!mi.has_spells())
        return "";

    formatted_string description;
    if (mi.type == MONS_PANDEMONIUM_LORD)
        description.cprintf("%s", panlord_desc.c_str());
    describe_spellset(monster_spellset(mi), nullptr, description, &mi);
    description.cprintf("\n");
    description.cprintf(jtrans_notrim("To read a description, press the key listed above.\n"));

    string desc = description.tostring();

    return desc + (desc.empty() ? "" : "\n");
}

static const char *_speed_description(int speed)
{
    // These thresholds correspond to the player mutations for fast and slow.
    ASSERT(speed != 10);
    if (speed < 7)
        return "extremely slowly";
    else if (speed < 8)
        return "very slowly";
    else if (speed < 10)
        return "slowly";
    else if (speed > 15)
        return "extremely quickly";
    else if (speed > 13)
        return "very quickly";
    else if (speed > 10)
        return "quickly";

    return "buggily";
}

static void _add_energy_to_string(int speed, int energy, string what,
                                  vector<string> &fast, vector<string> &slow)
{
    if (energy == 10)
        return;

    const int act_speed = (speed * 10) / energy;
    if (act_speed > 10)
        fast.push_back(adj_j(_speed_description(act_speed))
                       + jconj_verb(verb_j(what), JCONJ_PRES));
    if (act_speed < 10)
        slow.push_back(adj_j(_speed_description(act_speed))
                       + jconj_verb(verb_j(what), JCONJ_PRES));
}


/**
 * Print a bar of +s and .s representing a given stat to a provided stream.
 *
 * @param value[in]         The current value represented by the bar.
 * @param scale[in]         The value that each + and . represents.
 * @param name              The name of the bar.
 * @param result[in,out]    The stringstream to append to.
 * @param base_value[in]    The 'base' value represented by the bar. If
 *                          INT_MAX, is ignored.
 */
static void _print_bar(int value, int scale, string name,
                       ostringstream &result, int base_value = INT_MAX)
{
    if (base_value == INT_MAX)
        base_value = value;

    result << name << " ";

    const int display_max = value ? value : base_value;
    const bool currently_disabled = !value && base_value;

    if (currently_disabled)
      result << "(";

    for (int i = 0; i * scale < display_max; i++)
    {
        result << "+";
        if (i % 5 == 4)
            result << " ";
    }

    if (currently_disabled)
      result << ")";

#ifdef DEBUG_DIAGNOSTICS
    result << " (" << value << ")";
#endif

    if (currently_disabled)
    {
        result << jtrans_notrim(" (Normal") << ")";

#ifdef DEBUG_DIAGNOSTICS
        result << " (" << base_value << ")";
#endif
    }
}

/**
 * Append information about a given monster's HP to the provided stream.
 *
 * @param mi[in]            Player-visible info about the monster in question.
 * @param result[in,out]    The stringstream to append to.
 */
static void _describe_monster_hp(const monster_info& mi, ostringstream &result)
{
    result << jtrans_notrim("Max HP: ") << mi.get_max_hp_desc() << "\n";
}

/**
 * Append information about a given monster's AC to the provided stream.
 *
 * @param mi[in]            Player-visible info about the monster in question.
 * @param result[in,out]    The stringstream to append to.
 */
static void _describe_monster_ac(const monster_info& mi, ostringstream &result)
{
    // MAX_GHOST_EVASION + two pips (so with EV in parens it's the same)
    _print_bar(mi.ac, 5, " AC ", result);
    result << "\n";
}

/**
 * Append information about a given monster's EV to the provided stream.
 *
 * @param mi[in]            Player-visible info about the monster in question.
 * @param result[in,out]    The stringstream to append to.
 */
static void _describe_monster_ev(const monster_info& mi, ostringstream &result)
{
    _print_bar(mi.ev, 5, jtrans("EV"), result, mi.base_ev);
    result << "\n";
}

/**
 * Append information about a given monster's MR to the provided stream.
 *
 * @param mi[in]            Player-visible info about the monster in question.
 * @param result[in,out]    The stringstream to append to.
 */
static void _describe_monster_mr(const monster_info& mi, ostringstream &result)
{
    if (mi.res_magic() == MAG_IMMUNE)
    {
        result << jtrans_notrim("MR ∞\n");
        return;
    }

    const int bar_scale = MR_PIP;
    _print_bar(mi.res_magic(), bar_scale, tagged_jtrans("[abbr]", "MR"), result);
    result << "\n";
}


// Describe a monster's (intrinsic) resistances, speed and a few other
// attributes.
static string _monster_stat_description(const monster_info& mi)
{
    ostringstream result;

    result << "\n";

    _describe_monster_hp(mi, result);
    _describe_monster_ac(mi, result);
    _describe_monster_ev(mi, result);
    _describe_monster_mr(mi, result);

    result << "\n";

    resists_t resist = mi.resists();

    const mon_resist_flags resists[] =
    {
        MR_RES_ELEC,    MR_RES_POISON, MR_RES_FIRE,
        MR_RES_STEAM,   MR_RES_COLD,   MR_RES_ACID,
        MR_RES_ROTTING, MR_RES_NEG,    MR_RES_DAMNATION,
    };

    vector<string> extreme_resists;
    vector<string> high_resists;
    vector<string> base_resists;
    vector<string> suscept;

    for (mon_resist_flags rflags : resists)
    {
        int level = get_resist(resist, rflags);

        if (level != 0)
        {
            const string attackname = jtrans(_get_resist_name(rflags));
            if (rflags == MR_RES_DAMNATION)
                level = 3; // one level is immunity
            level = max(level, -1);
            level = min(level,  3);
            switch (level)
            {
                case -1:
                    suscept.emplace_back(attackname);
                    break;
                case 1:
                    base_resists.emplace_back(attackname);
                    break;
                case 2:
                    high_resists.emplace_back(attackname);
                    break;
                case 3:
                    extreme_resists.emplace_back(attackname);
                    break;
            }
        }
    }

    vector<string> resist_descriptions;
    if (!extreme_resists.empty())
    {
        const string tmp = make_stringf(jtransc("immune to {immune list}."),
              to_separated_line(extreme_resists.begin(),
                                extreme_resists.end(), "や", "、", "、").c_str());
        resist_descriptions.push_back(tmp);
    }
    if (!high_resists.empty())
    {
        const string tmp = make_stringf(jtransc("very resistant to {resist list}."),
              to_separated_line(high_resists.begin(),
                                high_resists.end(),  "や", "、", "、").c_str());
        resist_descriptions.push_back(tmp);
    }
    if (!base_resists.empty())
    {
        const string tmp = make_stringf(jtransc("resistant to {resist list}."),
              to_separated_line(base_resists.begin(),
                                base_resists.end(),  "や", "、", "、").c_str());
        resist_descriptions.push_back(tmp);
    }

    string pronoun = mi.pronoun_j(PRONOUN_SUBJECTIVE);
    string threat_level = tagged_jtrans("[threat]", _get_threat_desc(mi.threat));

    if (pronoun == jtrans("It"))
        pronoun = "このモンスター";
    if (mi.threat >= MTHRT_TOUGH)
        threat_level += "だ";

    if (mi.threat != MTHRT_UNDEF)
    {
        result << make_stringf(jtrans_notrimc("%s looks {threat level}.\n"),
                               pronoun.c_str(),
                               threat_level.c_str());
    }

    const string sep = "\n" + pronoun + "は";

    if (!resist_descriptions.empty())
    {
        result << make_stringf(jtrans_notrimc("%s is {resist desc list}.\n"),
                               pronoun.c_str(),
                               to_separated_line(resist_descriptions.begin(),
                                                 resist_descriptions.end(),
                                                 sep, sep, sep).c_str());
    }

    // Is monster susceptible to anything? (On a new line.)
    if (!suscept.empty())
    {
        result << make_stringf(jtrans_notrimc("%s is susceptible to {suscept list}.\n"),
                               pronoun.c_str(),
                               to_separated_line(suscept.begin(), suscept.end(),
                                                 sep, sep).c_str());
    }

    if (mons_class_flag(mi.type, M_STATIONARY)
        && !mons_is_tentacle_or_tentacle_segment(mi.type))
    {
        result << make_stringf(jtrans_notrimc("%s cannot move.\n"),
                               pronoun.c_str());
    }

    if (mons_class_flag(mi.type, M_COLD_BLOOD)
        && get_resist(resist, MR_RES_COLD) <= 0)
    {
        result << make_stringf(jtrans_notrimc("%s is cold-blooded and may be "
                                              "slowed by cold attacks.\n"),
                               pronoun.c_str());
    }

    // Seeing invisible.
    if (mi.can_see_invisible())
        result << make_stringf(jtrans_notrimc("%s can see invisible.\n"),
                               pronoun.c_str());

    // Echolocation, wolf noses, jellies, etc
    if (!mons_can_be_blinded(mi.type))
        result << make_stringf(jtrans_notrimc("%s is immune to blinding.\n"),
                               pronoun.c_str());
    // XXX: could mention "immune to dazzling" here, but that's spammy, since
    // it's true of such a huge number of monsters. (undead, statues, plants).
    // Might be better to have some place where players can see holiness &
    // information about holiness.......?


    // Unusual monster speed.
    const int speed = mi.base_speed();
    bool did_speed = false;
    if (speed != 10 && speed != 0)
    {
        did_speed = true;
        result << make_stringf(jtrans_notrimc("{pronoun} is {speed desc}"),
                               pronoun.c_str(),
                               jtransc(mi.speed_description()));
    }
    const mon_energy_usage def = DEFAULT_ENERGY;
    if (!(mi.menergy == def))
    {
        const mon_energy_usage me = mi.menergy;
        vector<string> fast, slow;
        if (!did_speed)
            result << pronoun << "は";
        _add_energy_to_string(speed, me.move, "covers ground", fast, slow);
        // since MOVE_ENERGY also sets me.swim
        if (me.swim != me.move)
            _add_energy_to_string(speed, me.swim, "swims", fast, slow);
        _add_energy_to_string(speed, me.attack, "attacks", fast, slow);
        if (mons_class_itemuse(mi.type) >= MONUSE_STARTING_EQUIPMENT)
            _add_energy_to_string(speed, me.missile, "shoots", fast, slow);
        _add_energy_to_string(
            speed, me.spell,
            mi.is_actual_spellcaster() ? "casts spells" :
            mi.is_priest()             ? "uses invocations"
                                       : "uses natural abilities", fast, slow);
        _add_energy_to_string(speed, me.special, "uses special abilities",
                              fast, slow);
        if (mons_class_itemuse(mi.type) >= MONUSE_STARTING_EQUIPMENT)
            _add_energy_to_string(speed, me.item, "uses items", fast, slow);

        const string pronoun_is = pronoun + "は";
        const string nlpronoun_is = jtrans_notrim(".\n") + pronoun_is;

        if (speed >= 10)
        {
            if (did_speed)
                result << jtrans_notrim(".\n");

            if (did_speed && fast.size() == 1)
                result << pronoun_is << fast[0] << jtrans_notrim(".\n");
            else if (!fast.empty())
            {
                if (did_speed)
                    result << jtrans(", ");
                result << comma_separated_line(fast.begin(), fast.end(),
                                               nlpronoun_is, nlpronoun_is)
                       << jtrans_notrim(".\n");
            }
            if (!slow.empty())
            {
                if (did_speed || !fast.empty())
                    result << jtrans(", but ");
                result << comma_separated_line(slow.begin(), slow.end(),
                                               nlpronoun_is, nlpronoun_is)
                       << jtrans_notrim(".\n");
            }
        }
        else if (speed < 10)
        {
            if (did_speed && slow.size() == 1)
                result << pronoun_is << slow[0] << jtrans_notrim(".\n");
            else if (!slow.empty())
            {
                if (did_speed)
                    result << jtrans(", ");
                result << comma_separated_line(slow.begin(), slow.end(),
                                               nlpronoun_is, nlpronoun_is)
                       << jtrans_notrim(".\n");
            }
            if (!fast.empty())
            {
                if (did_speed || !slow.empty())
                    result << jtrans(", but ");
                result << comma_separated_line(fast.begin(), fast.end(),
                                               nlpronoun_is, nlpronoun_is)
                       << jtrans_notrim(".\n");
            }
        }
    }
    else if (did_speed)
        result << jtrans_notrim(".\n");

    if (mi.type == MONS_SHADOW)
    {
        // Cf. monster::action_energy() in monster.cc.
        result << make_stringf(jtrans_notrimc("%s covers ground more"
                                              " quickly when invisible.\n"),
                               pronoun.c_str());
    }

    if (mi.airborne())
        result << make_stringf(jtrans_notrimc("%s can fly.\n"),
                               pronoun.c_str());

    // Unusual regeneration rates.
    if (!mi.can_regenerate())
        result << make_stringf(jtrans_notrimc("%s cannot regenerate.\n"),
                               pronoun.c_str());
    else if (mons_class_fast_regen(mi.type))
        result << make_stringf(jtrans_notrimc("%s regenerates quickly.\n"),
                               pronoun.c_str());

    // Size
    static const char * const sizes[] =
    {
        "tiny",
        "very small",
        "small",
        nullptr,     // don't display anything for 'medium'
        "large",
        "very large",
        "giant",
    };
    COMPILE_CHECK(ARRAYSZ(sizes) == NUM_SIZE_LEVELS);

    if (sizes[mi.body_size()])
    {
        result << make_stringf(jtrans_notrimc("{pronoun} is {size}.\n"),
                               pronoun.c_str(),
                               tagged_jtransc("[size]", sizes[mi.body_size()]));
    }

    result << _monster_attacks_description(mi);
    result << _monster_spells_description(mi);

    return result.str();
}

string serpent_of_hell_flavour(monster_type m)
{
    switch (m)
    {
    case MONS_SERPENT_OF_HELL_COCYTUS:
        return "cocytus";
    case MONS_SERPENT_OF_HELL_DIS:
        return "dis";
    case MONS_SERPENT_OF_HELL_TARTARUS:
        return "tartarus";
    default:
        return "gehenna";
    }
}

static string _get_unique_title(const string &key)
{
    return getMiscString(key + " title");
}

// Fetches the monster's database description and reads it into inf.
void get_monster_db_desc(const monster_info& mi, describe_info &inf,
                         bool &has_stat_desc, bool force_seen)
{
    string desc, desc_en, monname;
    if (inf.title.empty())
    {
        monname = mi.common_name_en(DESC_DBNAME);
        desc = jtrans(_get_unique_title(monname));
        desc_en = _get_unique_title(monname);

        if (desc_en.empty())
        {
            desc = mi.full_name(DESC_A);
            desc_en = uppercase_first(mi.full_name_en(DESC_A));
        }

        string spacer = _spacer(get_number_of_cols() - strwidth(desc)
                                                     - strwidth(desc_en) - 1);

        inf.title = desc + spacer + (desc != desc_en ? desc_en : "");
    }

    if (inf.title.empty())
        inf.title = mi.full_name(DESC_A);

    string db_name;

    if (mi.props.exists("dbname"))
        db_name = mi.props["dbname"].get_string();
    else if (mi.mname.empty())
        db_name = mi.db_name();
    else
        db_name = mi.full_name(DESC_PLAIN);

    if (mons_species(mi.type) == MONS_SERPENT_OF_HELL)
        db_name += " " + serpent_of_hell_flavour(mi.type);

    // This is somewhat hackish, but it's a good way of over-riding monsters'
    // descriptions in Lua vaults by using MonPropsMarker. This is also the
    // method used by set_feature_desc_long, etc. {due}
    if (!mi.description.empty())
        inf.body << "\n" << mi.description;
    // Don't get description for player ghosts.
    else if (mi.type != MONS_PLAYER_GHOST
             && mi.type != MONS_PLAYER_ILLUSION)
    {
        string description = getLongDescription(db_name);
        if (!description.empty())
            inf.body << "\n" << description;
    }

    // And quotes {due}
    if (!mi.quote.empty())
        inf.quote = mi.quote;
    else
        inf.quote = getQuoteString(db_name);

    string symbol;
    symbol += get_monster_data(mi.type)->basechar;
    if (isaupper(symbol[0]))
        symbol = "cap-" + symbol;

    string quote2;
    if (!mons_is_unique(mi.type))
    {
        string symbol_prefix = "__" + symbol + "_prefix";
        inf.prefix = getLongDescription(symbol_prefix);

        string symbol_suffix = "__" + symbol + "_suffix";
        quote2 = getQuoteString(symbol_suffix);
    }

    if (!inf.quote.empty() && !quote2.empty())
        inf.quote += "\n";
    inf.quote += quote2;

    const string it = mi.pronoun_j(PRONOUN_SUBJECTIVE);
    const string it_o = mi.pronoun_j(PRONOUN_OBJECTIVE);
    const string It = (it == jtrans("It") ? "このモンスター" : it);

    switch (mi.type)
    {
    case MONS_RED_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_MOTTLED_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_GREY_DRACONIAN:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_KNIGHT:
    {
        inf.body << "\n" << _describe_draconian(mi) << "\n";
        break;
    }

    case MONS_MONSTROUS_DEMONSPAWN:
    case MONS_GELID_DEMONSPAWN:
    case MONS_INFERNAL_DEMONSPAWN:
    case MONS_TORTUROUS_DEMONSPAWN:
    case MONS_BLOOD_SAINT:
    case MONS_WARMONGER:
    case MONS_CORRUPTER:
    case MONS_BLACK_SUN:
    {
        inf.body << "\n" << _describe_demonspawn(mi) << "\n";
        break;
    }

    case MONS_PLAYER_GHOST:
        inf.body << make_stringf(jtrans_notrimc("\nThe apparition of {desc}.\n"),
                                 get_ghost_description(mi).c_str());
        break;

    case MONS_PLAYER_ILLUSION:
        inf.body << make_stringf(jtrans_notrimc("\nAn illusion of {desc}.\n"),
                                 get_ghost_description(mi).c_str());
        break;

    case MONS_PANDEMONIUM_LORD:
        inf.body << "\n" << _describe_demon(mi.mname, mi.airborne()) << "\n";
        break;

    case MONS_MUTANT_BEAST:
        // vault renames get their own descriptions
        if (mi.mname.empty() || !mi.is(MB_NAME_REPLACE))
            inf.body << (inf.body.tellp() == 0 ? "" : "\n")
                     << _describe_mutant_beast(mi) << "\n";
        break;

    case MONS_BLOCK_OF_ICE:
        if (mi.is(MB_SLOWLY_DYING))
            inf.body << make_stringf(jtrans_notrimc("\nIt is quickly melting away.\n"),
                                     It.c_str());
        break;

    case MONS_PILLAR_OF_SALT:
        if (mi.is(MB_SLOWLY_DYING))
            inf.body << make_stringf(jtrans_notrimc("\nIt is quickly crumbling away.\n"),
                                     It.c_str());
        break;

    case MONS_PROGRAM_BUG:
        inf.body << "If this monster is a \"program bug\", then it's "
                "recommended that you save your game and reload. Please report "
                "monsters who masquerade as program bugs or run around the "
                "dungeon without a proper description to the authorities.\n";
        break;

    default:
        break;
    }

    if (!mons_is_unique(mi.type))
    {
        string symbol_suffix = "__";
        symbol_suffix += symbol;
        symbol_suffix += "_suffix";

        string suffix = getLongDescription(symbol_suffix)
                      + getLongDescription(symbol_suffix + "_examine");

        if (!suffix.empty())
            inf.body << "\n" << suffix;
    }

    const int curse_power = mummy_curse_power(mi.type);
    if (curse_power && !mi.is(MB_SUMMONED))
    {
        inf.body << make_stringf(jtrans_notrimc("{It} will inflict a %snecromantic curse on {its} foe when destroyed.\n"),
                                 It.c_str(),
                                 jtransc(curse_power > 10 ? "powerful " : ""),
                                 mi.pronoun_j(PRONOUN_POSSESSIVE).c_str());
    }

    // Get information on resistances, speed, etc.
    string result = _monster_stat_description(mi);
    if (!result.empty())
    {
        inf.body << result;
        has_stat_desc = true;
    }

    string pronoun = mi.pronoun_j(PRONOUN_SUBJECTIVE);
    if (pronoun == jtrans("It"))
        pronoun = "このモンスター";

    bool stair_use = false;
    if (!mons_class_can_use_stairs(mi.type))
    {
        inf.body << It << jtransln(" is incapable of using stairs.\n");
        stair_use = true;
    }

    if (mi.intel() <= I_BRAINLESS)
    {
        // Matters for Ely.
        inf.body << It << jtransln(" is mindless.\n");
    }
    else if (mi.intel() >= I_HUMAN)
    {
        // Matters for Yred, Gozag, Zin, TSO, Alistair....
        inf.body << It << jtransln(" is intelligent.\n");
    }

    if (mi.is(MB_CHAOTIC))
        inf.body << It << jtransln(" is vulnerable to silver and hated by Zin.\n");

    if (in_good_standing(GOD_ZIN, 0))
    {
        const int check = mi.hd - zin_recite_power();
        if (check >= 0)
            inf.body << It << jtrans(" is too strong to be recited to.");
        else if (check >= -5)
            inf.body << It << jtrans(" may be too strong to be recited to.");
        else
            inf.body << It << jtrans(" is weak enough to be recited to.");

        if (you.wizard)
        {
            inf.body << jtrans_notrim(" (Recite power:") << zin_recite_power()
                     << jtrans(", Hit dice:") << mi.hd << ")";
        }
        inf.body << "\n";
    }

    if (mi.is(MB_SUMMONED))
    {
        inf.body << jtrans_notrim(
                    "\nThis monster has been summoned, and is thus only "
                    "temporary. Killing {it_o} yields no experience, "
                    "nutrition or items");
        if (!stair_use)
            inf.body << jtrans(", and {it} is incapable of using stairs");
        inf.body << "ない。\n";
    }
    else if (mi.is(MB_PERM_SUMMON))
    {
        inf.body << jtrans_notrim(
                    "\nThis monster has been summoned in a durable way. "
                    "Killing {it} yields no experience, nutrition "
                    "or items, but {it} cannot be abjured.\n");
    }
    else if (mi.is(MB_NO_REWARD))
    {
        inf.body << jtrans_notrim(
                    "\nKilling this monster yields no experience, nutrition or"
                    " items.");
    }
    else if (mons_class_leaves_hide(mi.type))
    {
        inf.body << make_stringf(jtrans_notrimc("\nIf {it} is slain, it may be possible to "
                                               "recover {its} hide, which can be used as armour.\n"),
                                 mi.pronoun_j(PRONOUN_POSSESSIVE).c_str());
    }

    if (mi.is(MB_SUMMONED_CAPPED))
    {
        inf.body << jtrans_notrim(
                    "\nYou have summoned too many monsters of this kind to "
                    "sustain them all, and thus this one will shortly "
                    "expire.\n");
    }

#ifdef DEBUG_DIAGNOSTICS
    if (mi.pos.origin() || !monster_at(mi.pos))
        return; // not a real monster
    monster& mons = *monster_at(mi.pos);

    if (mons.has_originating_map())
    {
        inf.body << make_stringf("\nPlaced by map: %s",
                                 mons.originating_map().c_str());
    }

    inf.body << "\nMonster health: "
             << mons.hit_points << "/" << mons.max_hit_points << "\n";

    const actor *mfoe = mons.get_foe();
    inf.body << "Monster foe: "
             << (mfoe? mfoe->name(DESC_PLAIN, true)
                 : "(none)");

    vector<string> attitude;
    if (mons.friendly())
        attitude.emplace_back("friendly");
    if (mons.neutral())
        attitude.emplace_back("neutral");
    if (mons.good_neutral())
        attitude.emplace_back("good_neutral");
    if (mons.strict_neutral())
        attitude.emplace_back("strict_neutral");
    if (mons.pacified())
        attitude.emplace_back("pacified");
    if (mons.wont_attack())
        attitude.emplace_back("wont_attack");
    if (!attitude.empty())
    {
        string att = comma_separated_line(attitude.begin(), attitude.end(),
                                          "; ", "; ");
        if (mons.has_ench(ENCH_INSANE))
            inf.body << "; frenzied and insane (otherwise " << att << ")";
        else
            inf.body << "; " << att;
    }
    else if (mons.has_ench(ENCH_INSANE))
        inf.body << "; frenzied and insane";

    inf.body << "\n\nHas holiness: ";
    inf.body << holiness_description(mi.holi);
    inf.body << ".";

    const monster_spells &hspell_pass = mons.spells;
    bool found_spell = false;

    for (unsigned int i = 0; i < hspell_pass.size(); ++i)
    {
        if (!found_spell)
        {
            inf.body << "\n\nMonster Spells:\n";
            found_spell = true;
        }

        inf.body << "    " << i << ": "
                 << spell_title(hspell_pass[i].spell)
                 << " (";
        if (hspell_pass[i].flags & MON_SPELL_EMERGENCY)
            inf.body << "emergency, ";
        if (hspell_pass[i].flags & MON_SPELL_NATURAL)
            inf.body << "natural, ";
        if (hspell_pass[i].flags & MON_SPELL_MAGICAL)
            inf.body << "magical, ";
        if (hspell_pass[i].flags & MON_SPELL_WIZARD)
            inf.body << "wizard, ";
        if (hspell_pass[i].flags & MON_SPELL_PRIEST)
            inf.body << "priest, ";
        if (hspell_pass[i].flags & MON_SPELL_BREATH)
            inf.body << "breath, ";
        inf.body << (int) hspell_pass[i].freq << ")";
    }

    bool has_item = false;
    for (mon_inv_iterator ii(mons); ii; ++ii)
    {
        if (!has_item)
        {
            inf.body << "\n\nMonster Inventory:\n";
            has_item = true;
        }
        inf.body << "    " << ii.slot() << ": "
                 << ii->name(DESC_A, false, true);
    }

    if (mons.props.exists("blame"))
    {
        inf.body << "\n\nMonster blame chain:\n";

        const CrawlVector& blame = mons.props["blame"].get_vector();

        for (const auto &entry : blame)
            inf.body << "    " << entry.get_string() << "\n";
    }
#endif
}

int describe_monsters(const monster_info &mi, bool force_seen,
                      const string &footer)
{
    describe_info inf;
    bool has_stat_desc = false;
    get_monster_db_desc(mi, inf, has_stat_desc, force_seen);

    if (!footer.empty())
    {
        if (inf.footer.empty())
            inf.footer = footer;
        else
            inf.footer += "\n" + footer;
    }

#ifdef USE_TILE_WEB
    tiles_crt_control show_as_menu(CRT_MENU, "describe_monster");
#endif

    spell_scroller fs(monster_spellset(mi), &mi, nullptr);
    fs.add_text(inf.title);
    fs.add_text(inf.body.str(), false, get_number_of_cols() - 1);
    if (crawl_state.game_is_hints())
        fs.add_text(hints_describe_monster(mi, has_stat_desc).c_str());

    formatted_scroller qs;

    if (!inf.quote.empty())
    {
        fs.add_item_formatted_string(
                formatted_string::parse_string("\n" + jtrans(_toggle_message)));

        qs.add_text(inf.title);
        qs.add_text("\n" + inf.quote, false, get_number_of_cols() - 1);
        qs.add_item_formatted_string(
                formatted_string::parse_string("\n" + jtrans(_toggle_message)));
    }

    fs.add_item_formatted_string(formatted_string::parse_string(inf.footer));

    bool show_quote = false;
    while (true)
    {
        if (show_quote)
            qs.show();
        else
            fs.show();

        int keyin = (show_quote ? qs : fs).get_lastch();
        // this is never actually displayed to the player
        // we just use it to check whether we should toggle.
        if (_print_toggle_message(inf, keyin))
            show_quote = !show_quote;
        else
            return keyin;
    }
}

static const char* xl_rank_names[] =
{
    "weakling",
    "amateur",
    "novice",
    "journeyman",
    "adept",
    "veteran",
    "master",
    "legendary"
};

static string _xl_rank_name(const int xl_rank)
{
    const string rank = xl_rank_names[xl_rank];

    return rank;
}

string short_ghost_description(const monster *mon, bool abbrev)
{
    ASSERT(mons_is_pghost(mon->type));

    const ghost_demon &ghost = *(mon->ghost);
    const char* rank = xl_rank_names[ghost_level_to_rank(ghost.xl)];

    string desc = make_stringf("%s%sの%s", jtransc(rank),
                               species_name_jc(ghost.species),
                               get_job_name_jc(ghost.job));

    return desc;
}

// Describes the current ghost's previous owner. The caller must
// prepend "The apparition of" or whatever and append any trailing
// punctuation that's wanted.
string get_ghost_description(const monster_info &mi, bool concise)
{
    ostringstream gstr;

    const species_type gspecies = mi.i_ghost.species;

    gstr << tagged_jtrans("[skill]",
                          skill_title_by_rank(mi.i_ghost.best_skill,
                                              mi.i_ghost.best_skill_rank,
                                              gspecies,
                                              species_has_low_str(gspecies), mi.i_ghost.religion))
         << "として名の知れた"
         << jtrans(_xl_rank_name(mi.i_ghost.xl_rank))
         << species_name_j(gspecies)
         << "の"
         << get_job_name_j(mi.i_ghost.job);

    if (mi.i_ghost.religion != GOD_NO_GOD)
    {
        gstr << "にして"
             << god_name_j(mi.i_ghost.religion)
             << "の信徒である";
    }

    gstr << "『" << mi.mname << "』";

    return gstr.str();
}

void describe_skill(skill_type skill)
{
    ostringstream data;

#ifdef USE_TILE_WEB
    tiles_crt_control show_as_menu(CRT_MENU, "describe_skill");
#endif

    data << get_skill_description(skill, true);

    print_description(data.str());
    getchm();
}

#ifdef USE_TILE
string get_command_description(const command_type cmd, bool terse)
{
    string lookup = command_to_name(cmd);

    if (!terse)
        lookup += " verbose";

    string result = getLongDescription(lookup);
    if (result.empty())
    {
        if (!terse)
        {
            // Try for the terse description.
            result = get_command_description(cmd, true);
            if (!result.empty()) {
                // verbose/terseで文面共通のものがあるため
                // verboseの場合句点を付加
                if (!ends_with(result, "。"))
                    result += "。";

                return result;
            }
        }
        return command_to_name(cmd);
    }

    return result.substr(0, result.length() - 1);
}
#endif

void alt_desc_proc::nextline()
{
    ostr << "\n";
}

void alt_desc_proc::print(const string &str)
{
    ostr << str;
}

int alt_desc_proc::count_newlines(const string &str)
{
    return count(begin(str), end(str), '\n');
}

void alt_desc_proc::trim(string &str)
{
    int idx = str.size();
    while (--idx >= 0)
    {
        if (str[idx] != '\n')
            break;
    }
    str.resize(idx + 1);
}

bool alt_desc_proc::chop(string &str)
{
    int loc = -1;
    for (size_t i = 1; i < str.size(); i++)
        if (str[i] == '\n' && str[i-1] == '\n')
            loc = i;

    if (loc == -1)
        return false;

    str.resize(loc);
    return true;
}

void alt_desc_proc::get_string(string &str)
{
    str = replace_all(ostr.str(), "\n\n\n\n", "\n\n");
    str = replace_all(str, "\n\n\n", "\n\n");

    trim(str);
    while (count_newlines(str) > h)
    {
        if (!chop(str))
            break;
    }
}

/**
 * Provide auto-generated information about the given cloud type. Describe
 * opacity & related factors.
 *
 * @param cloud_type        The cloud_type in question.
 * @return e.g. "\nThis cloud is opaque; one tile will not block vision, but
 *      multiple will. \nClouds of this kind the player makes will vanish very
 *      quickly once outside the player's sight."
 */
string extra_cloud_info(cloud_type cloud_type)
{
    const bool opaque = is_opaque_cloud(cloud_type);
    const string opacity_info = !opaque ? "" :
        jtrans_notrim("\nThis cloud is opaque; one tile will not block vision, but "
                      "multiple will.");
    const string vanish_info
        = jtrans_notrim(make_stringf("\nClouds of this kind an adventurer makes will vanish "
                                     "%s once outside their sight.",
                                     (opaque ? "quickly" : "almost instantly")));
    return opacity_info + vanish_info;
}
