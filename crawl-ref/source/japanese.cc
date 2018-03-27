/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#include "AppHdr.h"
#include "database.h"
#include "english.h"
#include "itemprop-enum.h"
#include "japanese.h"
#include "stringutil.h"

const string conjugate_verb_j(const string &verb, bool /* ignore */)
{
    return verb_j(verb);
}

const string decline_pronoun_j(gender_type gender, pronoun_type variant)
{
    ASSERT_RANGE(gender, 0, NUM_GENDERS);
    ASSERT_RANGE(variant, 0, NUM_PRONOUN_CASES);

    const char *variant_tag = "buggy";
    switch (variant)
    {
    case PRONOUN_SUBJECTIVE: variant_tag = "subj"; break;
    case PRONOUN_POSSESSIVE: variant_tag = "poss"; break;
    case PRONOUN_REFLEXIVE:  variant_tag = "refl"; break;
    case PRONOUN_OBJECTIVE:  variant_tag = "obj"; break;
    default: break;
    }

    switch (gender)
    {
    case GENDER_NEUTER:
        return tagged_jtrans(make_stringf("[gender neuter %s]", variant_tag),
                             decline_pronoun(gender, variant));
    case GENDER_MALE:
        return tagged_jtrans(make_stringf("[gender male %s]", variant_tag),
                             decline_pronoun(gender, variant));
    case GENDER_FEMALE:
        return tagged_jtrans(make_stringf("[gender female %s]", variant_tag),
                             decline_pronoun(gender, variant));
    case GENDER_YOU:
        return tagged_jtrans(make_stringf("[gender you %s]", variant_tag),
                             decline_pronoun(gender, variant));
    default:
        return make_stringf("buggy pronoun: %d, %d",
                            gender, variant);
    }
}

const char * general_counter_suffix(const int size)
{
    if (size <= 9)
        return "つ";
    else
        return "個";
}

string jconj_verb(const string& verb, jconj conj)
{
    string v = verb;

    switch (conj)
    {
        // 必要に応じて随時追加
    case JCONJ_IRRE:  break;
    case JCONJ_CONT:  break;
    case JCONJ_TERM:  break;
    case JCONJ_ATTR:  break;
    case JCONJ_HYPO:  break;
    case JCONJ_IMPR:  break;
    case JCONJ_PERF:
        v = replace_all(v, "立てる", "立てた");
        v = replace_all(v, "鳴く", "鳴いた");
        v = replace_all(v, "放つ", "放った");
        v = replace_all(v, "吠える", "吠えた");
        break;
    case JCONJ_PASS:
        v = replace_all(v, "を追放した", "が追放された"); // _takenote_kill_verb()
        v = replace_all(v, "を中立化した", "が中立化させられた");
        v = replace_all(v, "を隷属させた", "が隷属させられた");
        v = replace_all(v, "をスライムに変えた", "がスライムに変えられた");
        v = replace_all(v, "を殺した", "が殺された");
        break;
    case JCONJ_PRES:
        v = replace_all(v, "した", "する");
        v = replace_all(v, "泳いだ", "泳ぐ");
        v = replace_all(v, "唱えた", "唱える");
        v = replace_all(v, "使った", "使う");
        v = replace_all(v, "を上げた", "を上げる");
        v = replace_all(v, "叫んだ", "叫ぶ");
        v = replace_all(v, "鳴いた", "鳴く");
        break;
    case JCONJ_PAST:
        v = replace_all(v, "いる", "いた");
        break;
    }

    return v;
}

/*
 * english.cc/thing_do_grammar()の代替
 * 英語処理が必要な箇所もあるためthing_do_grammarは残す
 */
string thing_do_grammar_j(description_level_type dtype, bool add_stop,
                        bool force_article, string desc)
{
    if (dtype == DESC_PLAIN || !force_article)
        return desc;

    switch (dtype)
    {
    case DESC_THE:
    case DESC_A:
        return desc;
    case DESC_NONE:
        return "";
    default:
        return desc;
    }
}

static const char * _counter_suffix_armour(const item_def& item)
{
    switch(item.sub_type) // see itemprop-enum.h::enum armour_type
    {
    case ARM_ROBE:
    case ARM_LEATHER_ARMOUR:
    case ARM_RING_MAIL:
    case ARM_SCALE_MAIL:
    case ARM_CHAIN_MAIL:
    case ARM_PLATE_ARMOUR:
    case ARM_CLOAK:
    case ARM_TROLL_LEATHER_ARMOUR:
    case ARM_FIRE_DRAGON_ARMOUR:
    case ARM_ICE_DRAGON_ARMOUR:
    case ARM_STEAM_DRAGON_ARMOUR:
    case ARM_MOTTLED_DRAGON_ARMOUR:
    case ARM_STORM_DRAGON_ARMOUR:
    case ARM_GOLD_DRAGON_ARMOUR:
    case ARM_SWAMP_DRAGON_ARMOUR:
    case ARM_PEARL_DRAGON_ARMOUR:
    case ARM_SHADOW_DRAGON_ARMOUR:
    case ARM_QUICKSILVER_DRAGON_ARMOUR:
    case ARM_CENTAUR_BARDING:
    case ARM_NAGA_BARDING:
        return "着";

    case ARM_HAT:
    case ARM_HELMET:
        return "つ";

    case ARM_GLOVES:
        return "組";

    case ARM_BOOTS:
        return "足";

    case ARM_BUCKLER:
    case ARM_SHIELD:
    case ARM_LARGE_SHIELD:
    case ARM_ANIMAL_SKIN:
#if TAG_MAJOR_VERSION == 34
    case ARM_TROLL_HIDE:
    case ARM_FIRE_DRAGON_HIDE:
    case ARM_ICE_DRAGON_HIDE:
    case ARM_STEAM_DRAGON_HIDE:
    case ARM_MOTTLED_DRAGON_HIDE:
    case ARM_STORM_DRAGON_HIDE:
    case ARM_GOLD_DRAGON_HIDE:
    case ARM_SWAMP_DRAGON_HIDE:
    case ARM_PEARL_DRAGON_HIDE:
    case ARM_SHADOW_DRAGON_HIDE:
    case ARM_QUICKSILVER_DRAGON_HIDE:
#endif
        return "枚";

    default:
        return "[buggy suffix: see _counter_suffix_armour()]";
    }
}

static const char * _counter_suffix_misc(const item_def& item)
{
    switch(item.sub_type) // see itemprop-enum.h::enum misc_item_type
    {
    case MISC_FAN_OF_GALES:
    case MISC_LAMP_OF_FIRE:
#if TAG_MAJOR_VERSION == 34
    case MISC_STONE_OF_TREMORS:
    case MISC_BUGGY_LANTERN_OF_SHADOWS:
#endif
    case MISC_HORN_OF_GERYON:
    case MISC_BOX_OF_BEASTS:
    case MISC_CRYSTAL_BALL_OF_ENERGY:
    case MISC_RUNE_OF_ZOT:
    case MISC_QUAD_DAMAGE:
    case MISC_ZIGGURAT:
        return "個";

    case MISC_DISC_OF_STORMS:
    case MISC_PHANTOM_MIRROR:
        return "枚";

    case MISC_DECK_OF_ESCAPE:
    case MISC_DECK_OF_DESTRUCTION:
    case MISC_DECK_OF_SUMMONING:
    case MISC_DECK_OF_PUNISHMENT:
#if TAG_MAJOR_VERSION == 34
    case MISC_DECK_OF_WONDERS:
    case MISC_DECK_OF_WAR:
    case MISC_DECK_OF_CHANGES:
    case MISC_DECK_OF_DEFENCE:
#endif
        return "組";

    case MISC_PHIAL_OF_FLOODS:
        return "本";

    case MISC_SACK_OF_SPIDERS:
        return "袋";

    default:
        return "[buggy suffix: see _counter_suffix_misc()]";
    }
}

static const char * _counter_suffix_missile(const item_def& item)
{
    switch(item.sub_type)
    {
    case MI_NEEDLE:
    case MI_ARROW:
    case MI_BOLT:
    case MI_JAVELIN:
    case MI_TOMAHAWK:
        return "本";

    case MI_STONE:
    case MI_LARGE_ROCK:
    case MI_SLING_BULLET:
        return "個";

    case MI_THROWING_NET:
        return "枚";

    default:
        return "[buggy suffix: see _counter_suffix_missile()]";
    }
}

static const char * _counter_suffix_weapon(const item_def& item)
{
    switch(item.sub_type) // see itemprop-enum.h
    {
    case WPN_CLUB:
    case WPN_WHIP:
#if TAG_MAJOR_VERSION == 34
    case WPN_HAMMER:
#endif
    case WPN_MACE:
    case WPN_MORNINGSTAR:
    case WPN_EVENINGSTAR:
    case WPN_GREAT_MACE:
    case WPN_SPEAR:
    case WPN_TRIDENT:
    case WPN_HALBERD:
    case WPN_GLAIVE:
    case WPN_BARDICHE:
    case WPN_BLOWGUN:
#if TAG_MAJOR_VERSION > 34
    case WPN_HUNTING_SLING:
    case WPN_FUSTIBALUS:
#endif
    case WPN_DEMON_WHIP:
    case WPN_GIANT_CLUB:
    case WPN_GIANT_SPIKED_CLUB:
    case WPN_DEMON_TRIDENT:
    case WPN_STAFF:
    case WPN_QUARTERSTAFF:
    case WPN_LAJATANG:
    case WPN_SACRED_SCOURGE:
    case WPN_TRISHULA:
        return "本";

    case WPN_FLAIL:
    case WPN_DIRE_FLAIL:
    case WPN_DAGGER:
    case WPN_QUICK_BLADE:
    case WPN_SHORT_SWORD:
    case WPN_RAPIER:
#if TAG_MAJOR_VERSION > 34
    case WPN_CUTLASS:
#endif
    case WPN_FALCHION:
    case WPN_LONG_SWORD:
    case WPN_SCIMITAR:
    case WPN_GREAT_SWORD:
    case WPN_DEMON_BLADE:
    case WPN_DOUBLE_SWORD:
    case WPN_TRIPLE_SWORD:
    case WPN_SCYTHE:
#if TAG_MAJOR_VERSION == 34
    case WPN_BLESSED_FALCHION:
    case WPN_BLESSED_LONG_SWORD:
    case WPN_BLESSED_SCIMITAR:
    case WPN_BLESSED_GREAT_SWORD:
#endif
    case WPN_EUDEMON_BLADE:
#if TAG_MAJOR_VERSION == 34
    case WPN_BLESSED_DOUBLE_SWORD:
    case WPN_BLESSED_TRIPLE_SWORD:
#endif
        return "振";

    case WPN_HAND_AXE:
    case WPN_WAR_AXE:
    case WPN_BROAD_AXE:
    case WPN_BATTLEAXE:
    case WPN_EXECUTIONERS_AXE:
        return "挺";

#if TAG_MAJOR_VERSION > 34
    case WPN_HAND_CROSSBOW:
#endif
    case WPN_ARBALEST:
#if TAG_MAJOR_VERSION > 34
    case WPN_TRIPLE_CROSSBOW:
#endif
        return "丁";

    case WPN_SHORTBOW:
    case WPN_LONGBOW:
        return "張";

    default:
        return "[buggy suffix: see _counter_suffix_weapon()]";
    }
}

/**
 * アイテム種別に応じた助数詞を返す
 */
const char * counter_suffix(const item_def &item)
{
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        return _counter_suffix_weapon(item);
    case OBJ_ARMOUR:
        return _counter_suffix_armour(item);
    case OBJ_MISCELLANY:
        return _counter_suffix_misc(item);
    case OBJ_MISSILES:
        return _counter_suffix_missile(item);

    case OBJ_POTIONS:
    case OBJ_WANDS:
    case OBJ_RODS:
    case OBJ_STAVES:
        return "本";

    case OBJ_FOOD:
    case OBJ_JEWELLERY:
    case OBJ_ORBS:
        return "個";

    case OBJ_SCROLLS:
        return "巻";

    case OBJ_GOLD:
        return "枚";

    case OBJ_BOOKS:
        return "冊";

    case OBJ_CORPSES:
        return "体";

    default:
        return "[buggy suffix: see couter_suffix()]";
    }
}

const string jpluralise(const string &name, const string &prefix, const string &suffix)
{
    return prefix + jtrans(name) + suffix;
}

/*
 * english.cc/apply_description()の代替
 */
string apply_description_j(description_level_type desc, const string &name,
                           int quantity, bool in_words)
{
    switch (desc)
    {
    case DESC_A:
        return quantity > 1 ? make_stringf("%d %s", quantity, jtransc(name))
                            : jtrans(name);
    case DESC_YOUR:
        return jtrans("your ") + jtrans(name);
    case DESC_THE:
    case DESC_PLAIN:
    default:
        return jtrans(name);
    }
}

string get_desc_quantity_j(const int quant, const int total, string whose)
{
    if (total == quant)
        return whose;
    else if (quant == 1)
        return whose + "のうちの一つ";
    else if (quant == 2)
        return whose + "のうちの二つ";
    else if (quant >= total * 3 / 4)
        return whose + "のほとんど";
    else
        return whose + "のうちいくつか";
}

/**
 * 0～27の範囲の数値を漢数字変換(ヒドラ系モンスター用)
 */
string jnumber_for_hydra_heads(int heads)
{
    ASSERT((0 < heads) && (heads <= 27)); // for Lernaean hydra

    string nums[10] = {"", "一", "二", "三", "四", "五", "六", "七", "八", "九"};
    string num;

    if(heads >= 20)
        num += nums[heads / 10];
    if(heads >= 10)
        num += "十";
    num += nums[heads % 10];

    return num;
}
