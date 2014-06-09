/**
 * @file
 * @brief Functions for eating and butchering.
**/

#include "AppHdr.h"

#include "food.h"

#include <sstream>

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "externs.h"
#include "options.h"

#include "artefact.h"
#include "cio.h"
#include "clua.h"
#include "command.h"
#include "database.h"
#include "delay.h"
#include "effects.h"
#include "env.h"
#include "hints.h"
#include "invent.h"
#include "items.h"
#include "itemname.h"
#include "itemprop.h"
#include "item_use.h"
#include "libutil.h"
#include "macro.h"
#include "mgen_data.h"
#include "message.h"
#include "misc.h"
#include "mon-death.h"
#include "mon-place.h"
#include "mon-util.h"
#include "mutation.h"
#include "output.h"
#include "player.h"
#include "player-equip.h"
#include "player-stats.h"
#include "potion.h"
#include "random.h"
#include "religion.h"
#include "godconduct.h"
#include "godabil.h"
#include "skills2.h"
#include "spl-util.h"
#include "state.h"
#include "stuff.h"
#include "transform.h"
#include "travel.h"
#include "xom.h"

static corpse_effect_type _determine_chunk_effect(corpse_effect_type chunktype,
                                                  bool rotten_chunk);
static int _contamination_ratio(corpse_effect_type chunk_effect);
static void _eat_chunk(item_def& food);
static void _eating(item_def &food);
static void _describe_food_change(int hunger_increment);
static bool _consume_corpse(int slot, bool invent);
static void _heal_from_food(int hp_amt, bool unrot = false);

/*
 *  BEGIN PUBLIC FUNCTIONS
 */
void make_hungry(int hunger_amount, bool suppress_msg,
                 bool magic)
{
    if (crawl_state.disables[DIS_HUNGER])
        return;

    if (crawl_state.game_is_zotdef() && you.is_undead != US_SEMI_UNDEAD)
    {
        you.hunger = HUNGER_DEFAULT;
        you.hunger_state = HS_SATIATED;
        return;
    }
#if TAG_MAJOR_VERSION == 34
    // Lich/tree form djinn don't get exempted from food costs: infinite
    // healing from channeling would be just too good.
    if (you.species == SP_DJINNI)
    {
        if (!magic)
            return;

        contaminate_player(hunger_amount * 4 / 3, true);
        return;
    }
#endif

    if (you_foodless())
        return;

    if (magic)
        hunger_amount = calc_hunger(hunger_amount);

    if (hunger_amount == 0 && !suppress_msg)
        return;

#ifdef DEBUG_DIAGNOSTICS
    set_redraw_status(REDRAW_HUNGER);
#endif

    you.hunger -= hunger_amount;

    if (you.hunger < 0)
        you.hunger = 0;

    // So we don't get two messages, ever.
    bool state_message = food_change();

    if (!suppress_msg && !state_message)
        _describe_food_change(-hunger_amount);
}

void lessen_hunger(int satiated_amount, bool suppress_msg)
{
    if (you_foodless())
        return;

    you.hunger += satiated_amount;

    if (you.hunger > HUNGER_MAXIMUM)
        you.hunger = HUNGER_MAXIMUM;

    // So we don't get two messages, ever.
    bool state_message = food_change();

    if (!suppress_msg && !state_message)
        _describe_food_change(satiated_amount);
}

void set_hunger(int new_hunger_level, bool suppress_msg)
{
    if (you_foodless())
        return;

    int hunger_difference = (new_hunger_level - you.hunger);

    if (hunger_difference < 0)
        make_hungry(-hunger_difference, suppress_msg);
    else if (hunger_difference > 0)
        lessen_hunger(hunger_difference, suppress_msg);
}

bool you_foodless(bool can_eat)
{
    return you.is_undead == US_UNDEAD
#if TAG_MAJOR_VERSION == 34
        || you.species == SP_DJINNI && !can_eat
#endif
        ;
}

bool you_foodless_normally()
{
    return you.is_undead == US_UNDEAD && you.form != TRAN_LICH
#if TAG_MAJOR_VERSION == 34
        || you.species == SP_DJINNI
#endif
        ;
}

/**
 * More of a "weapon_switch back from butchering" function, switching
 * to a weapon is done using the wield_weapon() code.
 * Special cases like staves of power or other special weps are taken
 * care of by calling wield_effects().    {gdl}
 */
void weapon_switch(int targ)
{
    // Give the player an option to abort.
    if (you.weapon() && !check_old_item_warning(*you.weapon(), OPER_WIELD))
        return;

    if (targ == -1) // Unarmed Combat.
    {
        // Already unarmed?
        if (!you.weapon())
            return;

        mpr("You switch back to your bare hands.");
    }
    else
    {
        // Possibly not valid anymore (dropped, etc.).
        if (!you.inv[targ].defined())
            return;

        // Already wielding this weapon?
        if (you.equip[EQ_WEAPON] == you.inv[targ].link)
            return;

        if (!can_wield(&you.inv[targ]))
        {
            mprf("Not switching back to %s.",
                 you.inv[targ].name(DESC_INVENTORY).c_str());
            return;
        }
    }

    // Unwield the old weapon and wield the new.
    // XXX This is a pretty dangerous hack;  I don't like it.--GDL
    //
    // Well yeah, but that's because interacting with the wielding
    // code is a mess... this whole function's purpose was to
    // isolate this hack until there's a proper way to do things. -- bwr
    if (you.weapon())
        unwield_item(false);

    if (targ != -1)
        equip_item(EQ_WEAPON, targ, false);

    if (Options.chunks_autopickup || you.species == SP_VAMPIRE)
        autopickup();

    // Same amount of time as normal wielding.
    // FIXME: this duplicated code is begging for a bug.
    if (you.weapon())
        you.time_taken /= 2;
    else                        // Swapping to empty hands is faster.
    {
        you.time_taken *= 3;
        you.time_taken /= 10;
    }

    you.wield_change = true;
    you.turn_is_over = true;
}

static bool _should_butcher(int corpse_id, bool bottle_blood = false)
{
    const item_def &corpse = mitm[corpse_id];

    if (is_forbidden_food(corpse)
        && (Options.confirm_butcher == CONFIRM_NEVER
            || !yesno("Desecrating this corpse would be a sin. Continue anyway?",
                      false, 'n')))
    {
        if (Options.confirm_butcher != CONFIRM_NEVER)
            canned_msg(MSG_OK);
        return false;
    }
    else if (!bottle_blood && you.species == SP_VAMPIRE
             && (can_bottle_blood_from_corpse(corpse.mon_type)
                 || mons_has_blood(corpse.mon_type) && !is_bad_food(corpse))
             && !you.has_spell(SPELL_SUBLIMATION_OF_BLOOD))
    {
        bool can_bottle = can_bottle_blood_from_corpse(corpse.mon_type);
        const string msg = make_stringf("You could drain this corpse's blood with <w>%s</w> instead%s. Continue anyway?",
                                        command_to_string(CMD_EAT).c_str(),
                                        can_bottle ? ", or drain it" : "");
        if (Options.confirm_butcher != CONFIRM_NEVER)
        {
            if (!yesno(msg.c_str(), true, 'n'))
            {
                canned_msg(MSG_OK);
                return false;
            }
        }
    }

    return true;
}

static bool _corpse_butchery(int corpse_id,
                             bool first_corpse = true,
                             bool bottle_blood = false)
{
    ASSERT(corpse_id != -1);

    const bool rotten = food_is_rotten(mitm[corpse_id]);

    if (!_should_butcher(corpse_id, bottle_blood))
        return false;

    // Start work on the first corpse we butcher.
    if (first_corpse)
        mitm[corpse_id].plus2++;

    int work_req = max(0, 4 - mitm[corpse_id].plus2);

    delay_type dtype = DELAY_BUTCHER;
    // Sanity checks.
    if (bottle_blood && !rotten
        && can_bottle_blood_from_corpse(mitm[corpse_id].mon_type))
    {
        dtype = DELAY_BOTTLE_BLOOD;
    }

    start_delay(dtype, work_req, corpse_id, mitm[corpse_id].special);

    you.turn_is_over = true;
    return true;
}

static int _corpse_badness(corpse_effect_type ce, const item_def &item,
                           bool wants_any)
{
    // Not counting poisonous chunks as useless here, caller must do that
    // themself.

    int contam = _contamination_ratio(ce);
    if (you.mutation[MUT_SAPROVOROUS] == 3)
        contam = -contam;

    // Arbitrarily lower the value of poisonous chunks: swapping resistances
    // is tedious.
    if (ce == CE_POISONOUS)
        contam = contam * 3 / 2;

    // Have uses that care about age but not quality?
    if (wants_any)
        contam /= 2;

    dprf("%s: to rot %d, contam %d -> badness %d",
         item.name(DESC_PLAIN).c_str(),
         item.special - ROTTING_CORPSE, contam,
         contam - 3 * item.special);

    // Being almost rotten has 480 badness, contamination usually 333.
    contam -= 3 * item.special;

    // Corpses your god gives penance for messing with are absolute last
    // priority.
    if (is_forbidden_food(item))
        contam += 10000;

    return contam;
}

int count_corpses_in_pack(bool blood_only)
{
    int num = 0;
    for (int i = 0; i < ENDOFPACK; ++i)
    {
        item_def &obj(you.inv[i]);

        if (!obj.defined())
            continue;

        // Only actually count corpses, not skeletons.
        if (obj.base_type != OBJ_CORPSES || obj.sub_type != CORPSE_BODY)
            continue;

        // Only saprovorous characters care about rotten food.
        if (food_is_rotten(obj) && (blood_only
                                    || !player_mutation_level(MUT_SAPROVOROUS)))
        {
            continue;
        }

        if (!blood_only || mons_has_blood(obj.mon_type))
            num++;
    }

    return num;
}

static bool _have_corpses_in_pack(bool remind, bool bottle_blood = false)
{
    const int num = count_corpses_in_pack(bottle_blood);

    if (num == 0)
        return false;

    string verb = (bottle_blood ? "bottle" : "butcher");

    string noun, pronoun;
    if (num == 1)
    {
        noun    = "corpse";
        pronoun = "it";
    }
    else
    {
        noun    = "corpses";
        pronoun = "them";
    }

    if (remind)
    {
        mprf("You might want to also %s the %s in your pack.", verb.c_str(),
             noun.c_str());
    }
    else
    {
        mprf("If you dropped the %s in your pack you could %s %s.",
             noun.c_str(), verb.c_str(), pronoun.c_str());
    }

    return true;
}

#ifdef TOUCH_UI
static string _butcher_menu_title(const Menu *menu, const string &oldt)
{
    return oldt;
}
#endif

bool butchery(int which_corpse, bool bottle_blood)
{
    if (you.visible_igrd(you.pos()) == NON_ITEM)
    {
        if (!_have_corpses_in_pack(false, bottle_blood))
            mpr("There isn't anything here!");
        return false;
    }

    bool wants_any = you.has_spell(SPELL_SUBLIMATION_OF_BLOOD);

    // First determine how many things there are to butcher.
    int num_corpses = 0;
    int corpse_id   = -1;
    int best_badness = INT_MAX;
    bool prechosen  = (which_corpse != -1);
    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (si->base_type == OBJ_CORPSES && si->sub_type == CORPSE_BODY)
        {
            if (bottle_blood && (food_is_rotten(*si)
                                 || !can_bottle_blood_from_corpse(si->mon_type)))
            {
                continue;
            }

            // Return pre-chosen corpse if it exists.
            if (prechosen && si->index() == which_corpse)
            {
                corpse_id = si->index();
                num_corpses = 1;
                break;
            }

            corpse_effect_type ce = _determine_chunk_effect(mons_corpse_effect(
                                                            si->mon_type),
                                                            food_is_rotten(*si));
            int badness = _corpse_badness(ce, *si, wants_any);
            if (ce == CE_POISONOUS)
                badness += 600;
            else if (ce == CE_MUTAGEN)
                badness += 1000;

            if (badness < best_badness)
                corpse_id = si->index(), best_badness = badness;
            num_corpses++;
        }
    }

    if (num_corpses == 0)
    {
        if (!_have_corpses_in_pack(false, bottle_blood))
        {
            mprf("There isn't anything to %s here.",
                 bottle_blood ? "bottle" : "butcher");
        }
        return false;
    }

    // Butcher pre-chosen corpse, if found, or if there is only one corpse.
    bool success = false;
    if (prechosen && corpse_id == which_corpse
        || num_corpses == 1 && Options.confirm_butcher != CONFIRM_ALWAYS
        || Options.confirm_butcher == CONFIRM_NEVER)
    {
        if (Options.confirm_butcher == CONFIRM_NEVER
            && !_should_butcher(corpse_id, bottle_blood))
        {
            mprf("There isn't anything suitable to %s here.",
                 bottle_blood ? "bottle" : "butcher");
            return false;
        }

        success = _corpse_butchery(corpse_id, true, bottle_blood);

        // Remind player of corpses in pack that could be butchered or
        // bottled.
        _have_corpses_in_pack(true, bottle_blood);
        return success;
    }

    // Now pick what you want to butcher. This is only a problem
    // if there are several corpses on the square.
    bool butcher_all   = false;
    bool first_corpse  = true;
#ifdef TOUCH_UI
    vector<const item_def*> meat;
    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (si->base_type != OBJ_CORPSES || si->sub_type != CORPSE_BODY)
            continue;

        if (bottle_blood && (food_is_rotten(*si)
                             || !can_bottle_blood_from_corpse(si->mon_type)))
        {
            continue;
        }
        meat.push_back(& (*si));
    }

    corpse_id = -1;
    vector<SelItem> selected =
        select_items(meat, bottle_blood ? "Choose a corpse to bottle"
                                        : "Choose a corpse to butcher",
                     false, MT_ANY, _butcher_menu_title);
    redraw_screen();
    for (int i = 0, count = selected.size(); i < count; ++i)
    {
        corpse_id = selected[i].item->index();
        if (_corpse_butchery(corpse_id, first_corpse, bottle_blood))
        {
            success = true;
            first_corpse = false;
        }
    }

#else
    int keyin;
    bool repeat_prompt = false;
    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (si->base_type != OBJ_CORPSES || si->sub_type != CORPSE_BODY)
            continue;

        if (bottle_blood && (food_is_rotten(*si)
                             || !can_bottle_blood_from_corpse(si->mon_type)))
        {
            continue;
        }

        if (butcher_all)
            corpse_id = si->index();
        else
        {
            corpse_id = -1;

            string corpse_name = si->name(DESC_A);

            // We don't need to check for undead because
            // * Mummies can't eat.
            // * Ghouls relish the bad things.
            // * Vampires won't bottle bad corpses.
            if (!you.is_undead)
                corpse_name = get_menu_colour_prefix_tags(*si, DESC_A);

            // Shall we butcher this corpse?
            do
            {
                mprf(MSGCH_PROMPT, "%s %s? [(y)es/(c)hop/(n)o/(a)ll/(q)uit/?]",
                     bottle_blood ? "Bottle" : "Butcher",
                     corpse_name.c_str());
                repeat_prompt = false;

                keyin = toalower(getchm(KMC_CONFIRM));
                switch (keyin)
                {
                case 'y':
                case 'c':
                case 'd':
                case 'a':
                    corpse_id = si->index();

                    if (keyin == 'a')
                        butcher_all = true;
                    break;

                case 'q':
                CASE_ESCAPE
                    canned_msg(MSG_OK);
                    return false;

                case '?':
                    show_butchering_help();
                    mesclr();
                    redraw_screen();
                    repeat_prompt = true;
                    break;

                default:
                    break;
                }
            }
            while (repeat_prompt);
        }

        if (corpse_id != -1)
        {
            if (_corpse_butchery(corpse_id, first_corpse, bottle_blood))
            {
                success = true;
                first_corpse = false;
            }
        }
    }
#endif
    if (!butcher_all && corpse_id == -1)
    {
        mprf("There isn't anything %s to %s here.",
             Options.confirm_butcher == CONFIRM_NEVER ? "suitable" : "else",
             bottle_blood ? "bottle" : "butcher");
    }

    if (success)
    {
        // Remind player of corpses in pack that could be butchered or
        // bottled.
        _have_corpses_in_pack(true, bottle_blood);
    }

    return success;
}

bool prompt_eat_inventory_item(int slot)
{
    if (inv_count() < 1)
    {
        canned_msg(MSG_NOTHING_CARRIED);
        return false;
    }

    int which_inventory_slot = slot;

    if (slot == -1)
    {
        which_inventory_slot =
                prompt_invent_item(you.species == SP_VAMPIRE ? "Drain what?"
                                                             : "Eat which item?",
                                   MT_INVLIST,
                                   you.species == SP_VAMPIRE ? (int)OSEL_VAMP_EAT
                                                             : OBJ_FOOD,
                                   true, true, true, 0, -1, NULL,
                                   OPER_EAT);

        if (prompt_failed(which_inventory_slot))
            return false;
    }

    // This conditional can later be merged into food::can_ingest() when
    // expanded to handle more than just OBJ_FOOD 16mar200 {dlb}
    item_def &item(you.inv[which_inventory_slot]);
    if (you.species != SP_VAMPIRE)
    {
        if (item.base_type != OBJ_FOOD)
        {
            mpr("You can't eat that!");
            return false;
        }
    }
    else
    {
        if (item.base_type != OBJ_CORPSES || item.sub_type != CORPSE_BODY)
        {
            mpr("You crave blood!");
            return false;
        }
    }

    if (!can_ingest(item, false))
        return false;

    eat_item(item);
    you.turn_is_over = true;

    return true;
}

static bool _eat_check(bool check_hunger = true, bool silent = false)
{
    if (you_foodless(true))
    {
        if (!silent)
        {
            mpr("You can't eat.");
            crawl_state.zero_turns_taken();
        }
        return false;
    }

    if (!check_hunger)
        return true;

    if (you.hunger_state >= HS_ENGORGED)
    {
        if (!silent)
        {
            mprf("You're too full to %s anything.",
                 you.species == SP_VAMPIRE ? "drain" : "eat");
            crawl_state.zero_turns_taken();
        }
        return false;
    }
    return true;
}

// [ds] Returns true if something was eaten.
bool eat_food(int slot)
{
    if (!_eat_check())
        return false;

    // Skip the prompts if we already know what we're eating.
    if (slot == -1)
    {
        int result = prompt_eat_chunks();
        if (result == 1 || result == -1)
            return result > 0;

        if (result != -2) // else skip ahead to inventory
        {
            if (you.visible_igrd(you.pos()) != NON_ITEM)
            {
                result = eat_from_floor(true);
                if (result == 1)
                    return true;
                if (result == -1)
                    return false;
            }
        }
    }

    return prompt_eat_inventory_item(slot);
}

//     END PUBLIC FUNCTIONS

static string _how_hungry()
{
    if (you.hunger_state > HS_SATIATED)
        return "full";
    else if (you.species == SP_VAMPIRE)
        return "thirsty";
    return "hungry";
}

// Must match the order of hunger_state_t enums
static constexpr int hunger_threshold[HS_ENGORGED + 1] =
    { HUNGER_STARVING, HUNGER_NEAR_STARVING, HUNGER_VERY_HUNGRY, HUNGER_HUNGRY,
      HUNGER_SATIATED, HUNGER_FULL, HUNGER_VERY_FULL, HUNGER_ENGORGED };

// "initial" is true when setting the player's initial hunger state on game
// start or load: in that case it's not really a change, so we suppress the
// state change message and don't identify rings or stimulate Xom.
bool food_change(bool initial)
{
    bool state_changed = false;
    bool less_hungry   = false;

    you.hunger = max(you_min_hunger(), you.hunger);
    you.hunger = min(you_max_hunger(), you.hunger);

    // Get new hunger state.
    hunger_state_t newstate = HS_STARVING;
    while (newstate < HS_ENGORGED && you.hunger > hunger_threshold[newstate])
        newstate = (hunger_state_t)(newstate + 1);

    if (newstate != you.hunger_state)
    {
        state_changed = true;
        if (newstate > you.hunger_state)
            less_hungry = true;

        you.hunger_state = newstate;
        set_redraw_status(REDRAW_HUNGER);

        if (newstate < HS_ENGORGED)
            interrupt_activity(AI_HUNGRY);

        if (you.species == SP_VAMPIRE)
        {
            if (newstate <= HS_SATIATED)
            {
                if (you.duration[DUR_BERSERK] > 1)
                {
                    mprf(MSGCH_DURATION, "Your blood-deprived body can't sustain "
                                         "your rage any longer.");
                    you.duration[DUR_BERSERK] = 1;
                }
                if (you.form != TRAN_NONE && you.form != TRAN_BAT
                    && you.duration[DUR_TRANSFORMATION] > 2 * BASELINE_DELAY)
                {
                    mprf(MSGCH_DURATION, "Your blood-deprived body can't sustain "
                                         "your transformation much longer.");
                    you.set_duration(DUR_TRANSFORMATION, 2);
                }
            }
            else if (you.form == TRAN_BAT
                     && you.duration[DUR_TRANSFORMATION] > 5)
            {
                print_stats();
                mprf(MSGCH_WARN, "Your blood-filled body can't sustain your "
                                 "transformation much longer.");

                // Give more time because suddenly stopping flying can be fatal.
                you.set_duration(DUR_TRANSFORMATION, 5);
            }
            else if (newstate == HS_ENGORGED && is_eating_corpse()) // Alive
            {
                print_stats();
                mpr("You can't stomach any more blood right now.");
            }
        }

        if (!initial)
        {
            string msg = "You ";
            switch (you.hunger_state)
            {
            case HS_STARVING:
                if (you.species == SP_VAMPIRE)
                    msg += "feel devoid of blood!";
                else
                    msg += "are starving!";

                mprf(MSGCH_FOOD, less_hungry, "%s", msg.c_str());

                learned_something_new(HINT_YOU_STARVING);
                you.check_awaken(500);
                break;

            case HS_NEAR_STARVING:
                if (you.species == SP_VAMPIRE)
                    msg += "feel almost devoid of blood!";
                else
                    msg += "are near starving!";

                mprf(MSGCH_FOOD, less_hungry, "%s", msg.c_str());

                learned_something_new(HINT_YOU_HUNGRY);
                break;

            case HS_VERY_HUNGRY:
            case HS_HUNGRY:
                msg += "are feeling ";
                if (you.hunger_state == HS_VERY_HUNGRY)
                    msg += "very ";
                msg += _how_hungry();
                msg += ".";

                mprf(MSGCH_FOOD, less_hungry, "%s", msg.c_str());

                learned_something_new(HINT_YOU_HUNGRY);
                break;

            default:
                return state_changed;
            }
        }
    }

    return state_changed;
}

// food_increment is positive for eating, negative for hungering
static void _describe_food_change(int food_increment)
{
    int magnitude = (food_increment > 0)?food_increment:(-food_increment);
    string msg;

    if (magnitude == 0)
        return;

    msg = "You feel ";

    if (magnitude <= 100)
        msg += "slightly ";
    else if (magnitude <= 350)
        msg += "somewhat ";
    else if (magnitude <= 800)
        msg += "quite a bit ";
    else
        msg += "a lot ";

    if ((you.hunger_state > HS_SATIATED) ^ (food_increment < 0))
        msg += "more ";
    else
        msg += "less ";

    msg += _how_hungry().c_str();
    msg += ".";
    mpr(msg.c_str());
}

static bool _player_can_eat_rotten_meat(bool need_msg = false)
{
    if (player_mutation_level(MUT_SAPROVOROUS))
        return true;

    if (need_msg)
        mpr("You refuse to eat that rotten meat.");

    return false;
}

bool eat_item(item_def &food)
{
    int link;

    if (in_inventory(food))
        link = food.link;
    else
    {
        link = item_on_floor(food, you.pos());
        if (link == NON_ITEM)
            return false;
    }

    if (food.base_type == OBJ_CORPSES && food.sub_type == CORPSE_BODY)
    {
        if (species_can_eat_corpses(you.species)
            && _consume_corpse(link, in_inventory(food)))
        {
            you.turn_is_over = true;
            return true;
        }
        else
            return false;
    }
    else if (food.sub_type == FOOD_CHUNK)
    {
        if (food_is_rotten(food) && !_player_can_eat_rotten_meat(true))
            return false;

        _eat_chunk(food);
    }
    else
        _eating(food);

    you.turn_is_over = true;

    if (in_inventory(food))
        dec_inv_item_quantity(link, 1);
    else
        dec_mitm_item_quantity(link, 1);

    return true;
}

// Returns which of two food items is older (true for first, else false).
class compare_by_freshness
{
public:
    bool operator()(const item_def *food1, const item_def *food2)
    {
        ASSERT(food1->base_type == OBJ_CORPSES || food1->base_type == OBJ_FOOD);
        ASSERT(food2->base_type == OBJ_CORPSES || food2->base_type == OBJ_FOOD);
        // ASSERT(food1->base_type == food2->base_type);

        if (is_inedible(*food1))
            return false;

        if (is_inedible(*food2))
            return true;

        // Permafood can last longest, skip it if possible.
        if (food1->base_type == OBJ_FOOD && food1->sub_type != FOOD_CHUNK)
            return false;
        if (food2->base_type == OBJ_FOOD && food2->sub_type != FOOD_CHUNK)
            return true;

        // At this point, we know both are corpses or chunks, edible
        // (not rotten, or player is saprovore).

        // Always offer poisonous/mutagenic chunks last.
        if (is_bad_food(*food1) && !is_bad_food(*food2))
            return false;
        if (is_bad_food(*food2) && !is_bad_food(*food1))
            return true;

        return food1->special < food2->special;
    }
};

#ifdef TOUCH_UI
static string _floor_eat_menu_title(const Menu *menu, const string &oldt)
{
    return oldt;
}
#endif
// Returns -1 for cancel, 1 for eaten, 0 for not eaten.
int eat_from_floor(bool skip_chunks)
{
    if (!_eat_check())
        return false;

    // Corpses should have been handled before.
    if (you.species == SP_VAMPIRE && skip_chunks)
        return 0;

    bool need_more = false;
    int unusable_corpse = 0;
    int inedible_food = 0;
    item_def wonteat;
    bool found_valid = false;

    vector<item_def*> food_items;
    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (you.species == SP_VAMPIRE)
        {
            if (si->base_type != OBJ_CORPSES || si->sub_type != CORPSE_BODY)
                continue;

            if (!mons_has_blood(si->mon_type))
            {
                unusable_corpse++;
                continue;
            }
        }
        else
        {
            if (si->base_type != OBJ_FOOD
                && (!species_can_eat_corpses(you.species)
                    || si->base_type != OBJ_CORPSES
                    || si->sub_type != CORPSE_BODY))
            {
                continue;
            }

            // Chunks should have been handled before.
            if (skip_chunks && si->sub_type == FOOD_CHUNK)
                continue;

            if (is_bad_food(*si))
                continue;
        }

        if (!skip_chunks && food_is_rotten(*si)
            && !_player_can_eat_rotten_meat())
        {
            unusable_corpse++;
            continue;
        }
        else if (!can_ingest(*si, true))
        {
            if (!inedible_food)
            {
                wonteat = *si;
                inedible_food++;
            }
            else
            {
                // Increase only if we're dealing with different subtypes.
                // FIXME: Use a common check for herbivorous/carnivorous
                //        dislikes, for e.g. "Blech! You need blood!"
                ASSERT(wonteat.defined());
                if (wonteat.sub_type != si->sub_type)
                    inedible_food++;
            }

            continue;
        }

        found_valid = true;
        food_items.push_back(&(*si));
    }

    if (found_valid)
    {
#ifdef TOUCH_UI
        vector<SelItem> selected =
            select_items(food_items,
                         you.species == SP_VAMPIRE ? "Drink blood from" : "Eat",
                         false, MT_SELONE, _floor_eat_menu_title);
        redraw_screen();
        for (int i = 0, count = selected.size(); i < count; ++i)
        {
            const item_def *item = selected[i].item;
            if (!check_warning_inscriptions(*item, OPER_EAT))
                break;

            if (can_ingest(*item, false))
                return eat_item(*item);
        }
#else
        sort(food_items.begin(), food_items.end(), compare_by_freshness());
        for (unsigned int i = 0; i < food_items.size(); ++i)
        {
            item_def *item = food_items[i];
            string item_name = get_menu_colour_prefix_tags(*item, DESC_A);

            mprf(MSGCH_PROMPT, "%s %s%s? (ye/n/q/i?)",
                 (you.species == SP_VAMPIRE ? "Drink blood from" : "Eat"),
                 ((item->quantity > 1) ? "one of " : ""),
                 item_name.c_str());

            int keyin = toalower(getchm(KMC_CONFIRM));
            switch (keyin)
            {
            case 'q':
            CASE_ESCAPE
                canned_msg(MSG_OK);
                return -1;
            case 'e':
            case 'y':
                if (!check_warning_inscriptions(*item, OPER_EAT))
                    break;

                if (can_ingest(*item, false))
                    return eat_item(*item);
                need_more = true;
                break;
            case 'i':
            case '?':
                // Directly skip ahead to inventory.
                return 0;
            default:
                // Else no: try next one.
                break;
            }
        }
#endif
    }
    else
    {
        // Give a message about why these food items can not actually be eaten.
        if (unusable_corpse)
        {
            if (you.species == SP_VAMPIRE)
            {
                mprf("%s devoid of blood.",
                     unusable_corpse == 1 ? "This corpse is"
                                          : "These corpses are");
            }
            else
                _player_can_eat_rotten_meat(true);
            need_more = true;
        }
        else if (inedible_food)
        {
            if (inedible_food == 1)
            {
                ASSERT(wonteat.defined());
                // Use the normal cannot ingest message.
                if (can_ingest(wonteat, false))
                {
                    mprf(MSGCH_DIAGNOSTICS, "Error: Can eat %s after all?",
                         wonteat.name(DESC_PLAIN).c_str());
                }
            }
            else // Several different food items.
                mpr("You refuse to eat these food items.");
            need_more = true;
        }
    }

    if (need_more)
        more();

    return 0;
}

bool eat_from_inventory()
{
    if (!_eat_check())
        return false;

    // Corpses should have been handled before.
    if (you.species == SP_VAMPIRE)
        return 0;

    int unusable_corpse = 0;
    int inedible_food = 0;
    item_def *wonteat = NULL;
    bool found_valid = false;

    vector<item_def *> food_items;
    for (int i = 0; i < ENDOFPACK; ++i)
    {
        if (!you.inv[i].defined())
            continue;

        item_def *item = &you.inv[i];
        if (you.species == SP_VAMPIRE)
        {
            if (item->base_type != OBJ_CORPSES || item->sub_type != CORPSE_BODY)
                continue;

            if (!mons_has_blood(item->mon_type))
            {
                unusable_corpse++;
                continue;
            }
        }
        else
        {
            // Chunks should have been handled before.
            if (item->base_type != OBJ_FOOD || item->sub_type == FOOD_CHUNK)
                continue;
        }

        if (is_bad_food(*item))
            continue;

        if (food_is_rotten(*item) && !_player_can_eat_rotten_meat())
        {
            unusable_corpse++;
            continue;
        }
        else if (!can_ingest(*item, true))
        {
            if (!inedible_food)
            {
                wonteat = item;
                inedible_food++;
            }
            else
            {
                // Increase only if we're dealing with different subtypes.
                // FIXME: Use a common check for herbivorous/carnivorous
                //        dislikes, for e.g. "Blech! You need blood!"
                ASSERT(wonteat->defined());
                if (wonteat->sub_type != item->sub_type)
                    inedible_food++;
            }
            continue;
        }

        found_valid = true;
        food_items.push_back(item);
    }

    if (found_valid)
    {
        sort(food_items.begin(), food_items.end(), compare_by_freshness());
        for (unsigned int i = 0; i < food_items.size(); ++i)
        {
            item_def *item = food_items[i];
            string item_name = get_menu_colour_prefix_tags(*item, DESC_A);

            mprf(MSGCH_PROMPT, "%s %s%s? (ye/n/q)",
                 (you.species == SP_VAMPIRE ? "Drink blood from" : "Eat"),
                 ((item->quantity > 1) ? "one of " : ""),
                 item_name.c_str());

            int keyin = toalower(getchm(KMC_CONFIRM));
            switch (keyin)
            {
            case 'q':
            CASE_ESCAPE
                canned_msg(MSG_OK);
                return false;
            case 'e':
            case 'y':
                if (can_ingest(*item, false))
                    return eat_item(*item);
                break;
            default:
                // Else no: try next one.
                break;
            }
        }
    }
    else
    {
        // Give a message about why these food items can not actually be eaten.
        if (unusable_corpse)
        {
            if (you.species == SP_VAMPIRE)
            {
                mprf("%s devoid of blood.",
                     unusable_corpse == 1 ? "The corpse you are carrying is"
                                          : "The corpses you are carrying are");
            }
            else
                _player_can_eat_rotten_meat(true);
        }
        else if (inedible_food)
        {
            if (inedible_food == 1)
            {
                ASSERT(wonteat->defined());
                // Use the normal cannot ingest message.
                if (can_ingest(*wonteat, false))
                {
                    mprf(MSGCH_DIAGNOSTICS, "Error: Can eat %s after all?",
                         wonteat->name(DESC_PLAIN).c_str());
                }
            }
            else // Several different food items.
                mpr("You refuse to eat these food items.");
        }
    }

    return false;
}

// Returns -1 for cancel, 1 for eaten, 0 for not eaten,
//         -2 for skip to inventory.
int prompt_eat_chunks(bool only_auto)
{
    bool can_chunk = true;
    bool auto_ration = false;
    // Only ghouls can eat chunks, vampires have their own thing.
    if (player_mutation_level(MUT_SAPROVOROUS) != 3
        && you.species != SP_VAMPIRE)
    {
        can_chunk = false;
        auto_ration = true;
    }

    // If we *know* the player can eat chunks, doesn't have the gourmand
    // effect and isn't hungry, don't prompt for chunks.
    if (you.species != SP_VAMPIRE
        && you.hunger_state >= HS_SATIATED + player_likes_chunks())
    {
        can_chunk = false;
    }

    bool found_valid = false;
    vector<item_def *> chunks;

    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (you.species == SP_VAMPIRE)
        {
            if (si->base_type != OBJ_CORPSES || si->sub_type != CORPSE_BODY)
                continue;

            if (!mons_has_blood(si->mon_type))
                continue;
        }
        // Don't autoeat fruit - it is more tactically useful than other food
        else if (si->base_type != OBJ_FOOD || si->sub_type == FOOD_FRUIT)
            continue;
        else if (si->sub_type == FOOD_CHUNK && !can_chunk)
            continue;
        else if (si->sub_type != FOOD_CHUNK && !auto_ration)
            continue;
        // Don't autoeat fruit - it is more tactically useful than other food
        else if (player_mutation_level(MUT_SAPROVOROUS) != 3)
        {
            int value = ::food_value(*si);
            if ((you.hunger + value) > HUNGER_MAXIMUM)
                continue;
        }

        if (food_is_rotten(*si) && !_player_can_eat_rotten_meat())
            continue;

        // Don't prompt for bad food types.
        if (is_bad_food(*si))
            continue;

        found_valid = true;
        chunks.push_back(&(*si));
    }

    // Then search through the inventory.
    for (int i = 0; i < ENDOFPACK; ++i)
    {
        if (!you.inv[i].defined())
            continue;

        item_def *item = &you.inv[i];
        if (you.species == SP_VAMPIRE)
        {
            if (item->base_type != OBJ_CORPSES || item->sub_type != CORPSE_BODY)
                continue;

            if (!mons_has_blood(item->mon_type))
                continue;
        }
        // Don't autoeat fruit - it is more tactically useful than other food
        else if (item->base_type != OBJ_FOOD || item->sub_type == FOOD_FRUIT)
            continue;
        else if (item->sub_type == FOOD_CHUNK && !can_chunk)
            continue;
        else if (item->sub_type != FOOD_CHUNK && !auto_ration)
            continue;
        else if (player_mutation_level(MUT_SAPROVOROUS) != 3)
        {
            int value = ::food_value(*item);
            if ((you.hunger + value) > HUNGER_MAXIMUM)
                continue;
        }

        if (food_is_rotten(*item) && !_player_can_eat_rotten_meat())
            continue;

        // Don't prompt for bad food types.
        if (is_bad_food(*item))
            continue;

        found_valid = true;
        chunks.push_back(item);
    }

    const bool easy_eat = Options.easy_eat_chunks || only_auto;

    if (found_valid)
    {
        sort(chunks.begin(), chunks.end(), compare_by_freshness());
        for (unsigned int i = 0; i < chunks.size(); ++i)
        {
            bool autoeat = false;
            item_def *item = chunks[i];
            string item_name = get_menu_colour_prefix_tags(*item, DESC_A);

            const bool bad = is_bad_food(*item);

            // Allow undead to use easy_eat, but not auto_eat, since the player
            // might not want to drink blood as a vampire and might want to save
            // chunks as a ghoul.
            if (easy_eat && !bad && i_feel_safe() && !(only_auto && you.is_undead))
            {
                // If this chunk is safe to eat, just do so without prompting.
                autoeat = true;
            }
            else if (only_auto)
                return 0;
            else
            {
                mprf(MSGCH_PROMPT, "%s %s%s? (ye/n/q/i?)",
                     (you.species == SP_VAMPIRE ? "Drink blood from" : "Eat"),
                     ((item->quantity > 1) ? "one of " : ""),
                     item_name.c_str());
            }

            int keyin = autoeat ? 'y' : toalower(getchm(KMC_CONFIRM));
            switch (keyin)
            {
            case 'q':
            CASE_ESCAPE
                canned_msg(MSG_OK);
                return -1;
            case 'i':
            case '?':
                // Skip ahead to the inventory.
                return -2;
            case 'e':
            case 'y':
                if (can_ingest(*item, false))
                {
                    if (autoeat)
                    {
                        mprf("%s %s%s.",
                             (you.species == SP_VAMPIRE ? "Drinking blood from"
                                                        : "Eating"),
                             ((item->quantity > 1) ? "one of " : ""),
                             item_name.c_str());
                    }

                    if (eat_item(*item))
                        return 1;
                    else
                        return 0;
                }
                break;
            default:
                // Else no: try next one.
                break;
            }
        }
    }

    return 0;
}

static const char *_chunk_flavour_phrase(bool likes_chunks)
{
    const char *phrase;
    const int level = player_mutation_level(MUT_SAPROVOROUS);

    switch (level)
    {
    case 1:
    case 2:
        phrase = "tastes unpleasant.";
        break;

    case 3:
        phrase = "tastes good.";
        break;

    default:
        phrase = "tastes terrible.";
        break;
    }

    if (likes_chunks)
        phrase = "tastes great.";
    else if (player_mutation_level(MUT_GOURMAND))
    {
        phrase = one_chance_in(1000) ? "tastes like chicken!"
                                     : "tastes great.";
    }

    return phrase;
}

void chunk_nutrition_message(int nutrition)
{
    int perc_nutrition = nutrition * 100 / CHUNK_BASE_NUTRITION;
    if (perc_nutrition < 15)
        mpr("That was extremely unsatisfying.");
    else if (perc_nutrition < 35)
        mpr("That was not very filling.");
}

static int _apply_herbivore_nutrition_effects(int nutrition)
{
    int how_herbivorous = player_mutation_level(MUT_HERBIVOROUS);

    while (how_herbivorous--)
        nutrition = nutrition * 75 / 100;

    return nutrition;
}

static int _chunk_nutrition(int likes_chunks)
{
    int nutrition = CHUNK_BASE_NUTRITION;

    if (you.hunger_state < HS_SATIATED + likes_chunks)
    {
        return likes_chunks ? nutrition
                            : _apply_herbivore_nutrition_effects(nutrition);
    }

    return _apply_herbivore_nutrition_effects(nutrition);
}

static void _say_chunk_flavour(bool likes_chunks)
{
    mprf("This raw flesh %s", _chunk_flavour_phrase(likes_chunks));
}

static int _contamination_ratio(corpse_effect_type chunk_effect)
{
    int sapro = player_mutation_level(MUT_SAPROVOROUS);
    int ratio = 0;
    switch (chunk_effect)
    {
    case CE_ROT:
        return 1000;
    case CE_POISON_CONTAM:
    case CE_CONTAMINATED:
        switch (sapro)
        {
        default: ratio =  500; break; // including sapro 3 (contam is good)
        case 1:  ratio =  100; break;
        case 2:  ratio =   33; break;
        }
        break;
    case CE_ROTTEN:
        switch (sapro)
        {
        default: ratio = 1000; break;
        case 1:  ratio =  200; break;
        case 2:  ratio =   66; break;
        }
        break;
    default:
        break;
    }

    // Trolls get clean meat effects from contaim meat.
    if (you.gourmand())
        ratio = 0;

    return ratio;
}

// Never called directly - chunk_effect values must pass
// through food::_determine_chunk_effect() first. {dlb}:
static void _eat_chunk(item_def& food)
{
    const bool cannibal  = is_player_same_genus(food.mon_type);
    const int intel      = mons_class_intel(food.mon_type) - I_ANIMAL;
    const bool rotten    = food_is_rotten(food);
    const bool orc       = (mons_genus(food.mon_type) == MONS_ORC);
    const bool holy      = (mons_class_holiness(food.mon_type) == MH_HOLY);
    corpse_effect_type chunk_effect = mons_corpse_effect(food.mon_type);
    chunk_effect = _determine_chunk_effect(chunk_effect, rotten);

    int likes_chunks  = player_likes_chunks(true);
    int nutrition     = _chunk_nutrition(likes_chunks);
    bool suppress_msg = false; // do we display the chunk nutrition message?
    bool do_eat       = false;

    if (you.species == SP_GHOUL)
    {
        nutrition    = CHUNK_BASE_NUTRITION;
        suppress_msg = true;
    }

    switch (chunk_effect)
    {
    case CE_MUTAGEN:
        mpr("This meat tastes really weird.");
        mutate(RANDOM_MUTATION, "mutagenic meat");
        did_god_conduct(DID_DELIBERATE_MUTATING, 10);
        xom_is_stimulated(100);
        break;

    case CE_ROT:
        you.rot(&you, 10 + random2(10));
        if (you.sicken(50 + random2(100)))
            xom_is_stimulated(random2(100));
        break;

    case CE_ROTTEN:
    case CE_CONTAMINATED:
    case CE_CLEAN:
    {
        int contam = _contamination_ratio(chunk_effect);
        if (player_mutation_level(MUT_SAPROVOROUS) == 3)
        {
            mprf("This %s flesh tastes %s!",
                 chunk_effect == CE_ROTTEN   ? "rotting"   : "raw",
                 x_chance_in_y(contam, 1000) ? "delicious" : "good");
            if (you.species == SP_GHOUL)
            {
                int hp_amt = 1 + random2(5) + random2(1 + you.experience_level);
                if (!x_chance_in_y(contam + 4000, 5000))
                    hp_amt = 0;
                _heal_from_food(hp_amt, !one_chance_in(4));
            }
        }
        else
        {
            _say_chunk_flavour(likes_chunks);

            nutrition = nutrition * (1000 - contam) / 1000;
        }

        do_eat = true;
        break;
    }

    case CE_POISONOUS:
    case CE_POISON_CONTAM: // _determine_chunk_effect should never return this
    case CE_NOCORPSE:
        mprf(MSGCH_ERROR, "This flesh (%d) tastes buggy!", chunk_effect);
        break;
    }

    if (cannibal)
        did_god_conduct(DID_CANNIBALISM, 10);
    else if (intel > 0)
        did_god_conduct(DID_EAT_SOULED_BEING, intel);

    if (orc)
        did_god_conduct(DID_DESECRATE_ORCISH_REMAINS, 2);
    if (holy)
        did_god_conduct(DID_DESECRATE_HOLY_REMAINS, 2);

    if (do_eat)
    {
        dprf("nutrition: %d", nutrition);
        zin_recite_interrupt();
        start_delay(DELAY_EAT, food_turns(food) - 1,
                    (suppress_msg) ? 0 : nutrition, -1);
        lessen_hunger(nutrition, true);
    }
}

static void _eating(item_def& food)
{
    int food_value = ::food_value(food);
    ASSERT(food_value > 0);

    int duration = food_turns(food) - 1;

    // use delay.parm3 to figure out whether to output "finish eating"
    zin_recite_interrupt();
    start_delay(DELAY_EAT, duration, 0, food.sub_type, duration);

    lessen_hunger(food_value, true);
}

// Handle messaging at the end of eating.
// Some food types may not get a message.
void finished_eating_message(int food_type)
{
    bool herbivorous = player_mutation_level(MUT_HERBIVOROUS) > 0;
    bool carnivorous = player_mutation_level(MUT_CARNIVOROUS) > 0;

    if (herbivorous)
    {
        if (food_is_meaty(food_type))
        {
            mpr("Blech - you need greens!");
            return;
        }
    }
    else
    {
        switch (food_type)
        {
        case FOOD_MEAT_RATION:
            mpr("That meat ration really hit the spot!");
            return;
        case FOOD_BEEF_JERKY:
            mprf("That beef jerky was %s!",
                 one_chance_in(4) ? "jerk-a-riffic"
                                  : "delicious");
            return;
        default:
            break;
        }
    }

    if (carnivorous)
    {
        if (food_is_veggie(food_type))
        {
            mpr("Blech - you need meat!");
            return;
        }
    }
    else
    {
        switch (food_type)
        {
        case FOOD_BREAD_RATION:
            mpr("That bread ration really hit the spot!");
            return;
        case FOOD_FRUIT:
        {
            string taste = getMiscString("eating_fruit");
            if (taste.empty())
                taste = "Eugh, buggy fruit.";
            mprf("%s", taste.c_str());
            break;
        }
        default:
            break;
        }
    }

    switch (food_type)
    {
    case FOOD_ROYAL_JELLY:
        mpr("That royal jelly was delicious!");
        break;
    case FOOD_PIZZA:
    {
        string taste = getMiscString("eating_pizza");
        if (taste.empty())
            taste = "Bleh, bug pizza.";
        mprf("%s", taste.c_str());
        break;
    }
    default:
        break;
    }
}

// Divide full nutrition by duration, so that each turn you get the same
// amount of nutrition. Also, experimentally regenerate 1 hp per feeding turn
// - this is likely too strong.
// feeding is -1 at start, 1 when finishing, and 0 else

// Here are some values for nutrition (quantity * 1000) and duration:
//    max_chunks      quantity    duration
//     1               1           1
//     2               1           1
//     3               1           2
//     4               1           2
//     5               1           2
//     6               2           3
//     7               2           3
//     8               2           3
//     9               2           4
//    10               2           4
//    12               3           5
//    15               3           5
//    20               4           6
//    25               4           6
//    30               5           7

void nutrition_per_turn(const item_def &corpse, int feeding)
{
    const monster_type mons_type = corpse.mon_type;
    const corpse_effect_type chunk_type =
        _determine_chunk_effect(mons_corpse_effect(mons_type), false);

    // Duration depends on corpse weight.
    const int max_chunks = get_max_corpse_chunks(mons_type);
    int chunk_amount     = 1 + max_chunks/3;
        chunk_amount     = stepdown_value(chunk_amount, 6, 6, 12, 12);

    // Add 1 for the artificial extra call at the start of draining.
    const int duration   = 1 + chunk_amount;

    // Use number of potions per corpse to calculate total nutrition, which
    // then gets distributed over the entire duration.
    int food_value = CHUNK_BASE_NUTRITION
                     * num_blood_potions_from_corpse(mons_type, chunk_type);

    bool start_feeding   = false;
    bool end_feeding     = false;

    if (feeding < 0)
        start_feeding = true;
    else if (feeding > 0)
        end_feeding = true;

    if (you.species != SP_VAMPIRE)
    {
        int contam = _contamination_ratio(chunk_type);
        if (start_feeding)
        {
            mprf("This %s flesh tastes %s!",
                 chunk_type == CE_ROTTEN   ? "rotting"   : "raw",
                 x_chance_in_y(contam, 1000) ? "delicious" : "good");
        }
        if (you.species == SP_GHOUL && end_feeding)
        {
            int hp_amt = 1 + random2(5) + random2(1 + you.experience_level);
            if (!x_chance_in_y(contam + 4000, 5000))
                hp_amt = 0;
            _heal_from_food(hp_amt, !one_chance_in(4));
        }
    }
    else switch (chunk_type)
    {
        case CE_CLEAN:
            if (start_feeding)
            {
                mprf("This %sblood tastes delicious!",
                     mons_class_flag(mons_type, M_WARM_BLOOD) ? "warm "
                                                              : "");
            }
            else if (end_feeding && corpse.special > 150)
                _heal_from_food(1);
            break;

        case CE_CONTAMINATED:
            food_value /= 2;
            if (start_feeding)
                mpr("Somehow this blood was not very filling!");
            else if (end_feeding && corpse.special > 150)
                _heal_from_food(1);
            break;

        case CE_MUTAGEN:
            food_value /= 2;
            if (start_feeding)
                mpr("This blood tastes really weird!");
            mutate(RANDOM_MUTATION, "mutagenic blood");
            did_god_conduct(DID_DELIBERATE_MUTATING, 10);
            xom_is_stimulated(100);
            // Sometimes heal by one hp.
            if (end_feeding && corpse.special > 150 && coinflip())
                _heal_from_food(1);
            break;

        case CE_ROT:
            you.rot(&you, 5 + random2(5));
            if (you.sicken(50 + random2(100)))
                xom_is_stimulated(random2(100));
            stop_delay();
            break;

        case CE_POISONOUS:
        case CE_POISON_CONTAM:
        case CE_ROTTEN:
        case CE_NOCORPSE:
            mprf(MSGCH_ERROR, "This blood (%d) tastes buggy!", chunk_type);
            return;
    }

    if (!end_feeding)
        lessen_hunger(food_value / duration, !start_feeding);
}

/**
 * Will corpses from monsters of this type be potentially edible?
 *
 * For permafood-spawning purposes.
 *
 * @param mc    The type of monster being considered.
 * @return      Whether the monster type is edible for the player.
 */
bool monster_maybe_edible(monster_type mc)
{
    const corpse_effect_type ce = mons_corpse_effect(mc);
    return ce < CE_MUTAGEN
           && (ce != CE_ROT || you.res_rotting(false));
}

bool is_bad_food(const item_def &food)
{
    return is_poisonous(food) || is_mutagenic(food)
           || is_forbidden_food(food) || causes_rot(food);
}

// Returns true if a food item (or corpse) is poisonous AND the player is not
// (known to be) poison resistant.
bool is_poisonous(const item_def &food)
{
    if (food.base_type != OBJ_FOOD && food.base_type != OBJ_CORPSES)
        return false;

    if (player_res_poison(false) > 0)
        return false;

    return chunk_is_poisonous(mons_corpse_effect(food.mon_type));
}

// Returns true if a food item (or corpse) is mutagenic.
bool is_mutagenic(const item_def &food)
{
    if (food.base_type != OBJ_FOOD && food.base_type != OBJ_CORPSES)
        return false;

    // Has no effect on ghouls.
    if (you.species == SP_GHOUL)
        return false;

    return mons_corpse_effect(food.mon_type) == CE_MUTAGEN;
}

// Returns true if a food item (or corpse) is contaminated and thus
// gives less nutrition.
bool is_contaminated(const item_def &food)
{
    if ((food.base_type != OBJ_FOOD || food.sub_type != FOOD_CHUNK)
            && food.base_type != OBJ_CORPSES)
    {
        return false;
    }

    const corpse_effect_type chunk_type = mons_corpse_effect(food.mon_type);
    return chunk_type == CE_CONTAMINATED
           || (player_res_poison(false) > 0 && chunk_type == CE_POISON_CONTAM)
           || food_is_rotten(food)
              && player_mutation_level(MUT_SAPROVOROUS) < 3;
}

// Returns true if a food item (or corpse) will cause rotting.
bool causes_rot(const item_def &food)
{
    if (food.base_type != OBJ_FOOD && food.base_type != OBJ_CORPSES)
        return false;

    // Has no effect on ghouls.
    if (you.species == SP_GHOUL)
        return false;

    return mons_corpse_effect(food.mon_type) == CE_ROT;
}

// Returns true if an item of basetype FOOD or CORPSES cannot currently
// be eaten (respecting species and mutations set).
bool is_inedible(const item_def &item)
{
    // Mummies and liches don't eat.
    if (you_foodless(true))
        return true;

    if (food_is_rotten(item)
        && !player_mutation_level(MUT_SAPROVOROUS))
    {
        return true;
    }

    if (item.base_type == OBJ_FOOD
        && !can_ingest(item, true, false))
    {
        return true;
    }

    if (item.base_type == OBJ_CORPSES
        && (item.sub_type == CORPSE_SKELETON
            || you.species == SP_VAMPIRE && !mons_has_blood(item.mon_type)))
    {
        return true;
    }

    return false;
}

// As we want to avoid autocolouring the entire food selection, this should
// be restricted to the absolute highlights, even though other stuff may
// still be edible or even delicious.
bool is_preferred_food(const item_def &food)
{
    // Mummies and liches don't eat.
    if (you_foodless(true))
        return false;

    // Vampires don't really have a preferred food type, but they really
    // like blood potions.
    if (you.species == SP_VAMPIRE)
        return is_blood_potion(food);

    if (food.base_type == OBJ_POTIONS && food.sub_type == POT_PORRIDGE
        && item_type_known(food)
#if TAG_MAJOR_VERSION == 34
        && you.species != SP_DJINNI
#endif
        )
    {
        return !player_mutation_level(MUT_CARNIVOROUS);
    }

    if (food.base_type != OBJ_FOOD)
        return false;

    // Poisoned, mutagenic, etc. food should never be marked as "preferred".
    if (is_bad_food(food))
        return false;

    // Ghouls specifically like rotten food.
    if (you.species == SP_GHOUL)
        return food_is_rotten(food);

    if (player_mutation_level(MUT_CARNIVOROUS) == 3)
        return food_is_meaty(food.sub_type);

    if (player_mutation_level(MUT_HERBIVOROUS) == 3)
        return food_is_veggie(food.sub_type);

    // No food preference.
    return false;
}

bool is_forbidden_food(const item_def &food)
{
    if (food.base_type != OBJ_CORPSES
        && (food.base_type != OBJ_FOOD || food.sub_type != FOOD_CHUNK))
    {
        return false;
    }

    // Some gods frown upon cannibalistic behaviour.
    if (god_hates_cannibalism(you.religion)
        && is_player_same_genus(food.mon_type))
    {
        return true;
    }

    // Holy gods do not like it if you are eating holy creatures
    if (is_good_god(you.religion)
        && mons_class_holiness(food.mon_type) == MH_HOLY)
    {
        return true;
    }

    // Zin doesn't like it if you eat beings with a soul.
    if (you_worship(GOD_ZIN) && mons_class_intel(food.mon_type) >= I_NORMAL)
        return true;

    // Everything else is allowed.
    return false;
}

bool can_ingest(const item_def &food, bool suppress_msg, bool check_hunger)
{
    if (check_hunger)
    {
        if (is_poisonous(food))
        {
            if (!suppress_msg)
                mpr("It contains deadly poison!");
            return false;
        }
        if (causes_rot(food))
        {
            if (!suppress_msg)
                mpr("It is caustic! Not only inedible but also greatly harmful!");
            return false;
        }
    }
    // special case mutagenic chunks to skip hunger checks, as they don't give
    // nutrition and player can get hungry by using spells etc. anyway
    return can_ingest(food.base_type, food.sub_type, suppress_msg,
                      is_mutagenic(food) ? false : check_hunger,
                      food_is_rotten(food));
}

bool can_ingest(int what_isit, int kindof_thing, bool suppress_msg,
                bool check_hunger, bool rotten)
{
    bool survey_says = false;

    // [ds] These redundant checks are now necessary - Lua might be calling us.
    if (!_eat_check(check_hunger, suppress_msg))
        return false;

    if (you.species == SP_VAMPIRE)
    {
        if (what_isit == OBJ_CORPSES && kindof_thing == CORPSE_BODY)
            return true;

        if (what_isit == OBJ_POTIONS
            && (kindof_thing == POT_BLOOD
                || kindof_thing == POT_BLOOD_COAGULATED))
        {
            return true;
        }

        if (!suppress_msg)
            mpr("Blech - you need blood!");

        return false;
    }

    bool ur_carnivorous = player_mutation_level(MUT_CARNIVOROUS) == 3;
    bool ur_herbivorous = player_mutation_level(MUT_HERBIVOROUS) == 3;

    // ur_chunkslover not defined in terms of ur_carnivorous because
    // a player could be one and not the other IMHO - 13mar2000 {dlb}
    bool ur_chunkslover = check_hunger ? you.hunger_state < HS_SATIATED
                          + player_likes_chunks() : true;

    switch (what_isit)
    {
    case OBJ_FOOD:
    {
        if (you.species == SP_VAMPIRE)
        {
            if (!suppress_msg)
                mpr("Blech - you need blood!");
            return false;
        }

        if (food_is_veggie(kindof_thing))
        {
            if (ur_carnivorous)
            {
                if (!suppress_msg)
                    mpr("Sorry, you're a carnivore.");
                return false;
            }
            else
                return true;
        }
        else if (food_is_meaty(kindof_thing))
        {
            if (ur_herbivorous)
            {
                if (!suppress_msg)
                    mpr("Sorry, you're a herbivore.");
                return false;
            }
            else if (kindof_thing == FOOD_CHUNK)
            {
                if (player_mutation_level(MUT_SAPROVOROUS) != 3)
                    return false;

                if (rotten && !_player_can_eat_rotten_meat(!suppress_msg))
                    return false;

                if (ur_chunkslover)
                    return true;

                if (you_min_hunger() == you_max_hunger())
                    return true;

                if (!suppress_msg)
                    mpr("You aren't quite hungry enough to eat that!");

                return false;
            }
        }
        // Any food types not specifically handled until here (e.g. meat
        // rations for non-herbivores) are okay.
        return true;
    }

    case OBJ_CORPSES:
        if (you.species == SP_VAMPIRE || you.species == SP_GHOUL)
        {
            if (kindof_thing == CORPSE_BODY)
                return true;
            else
            {
                if (!suppress_msg)
                {
                    mprf("Blech - you need %s!",
                         you.species == SP_VAMPIRE ? "blood" : "meat");
                }
                return false;
            }
        }
        return false;

    case OBJ_POTIONS: // called by lua
        if (get_ident_type(OBJ_POTIONS, kindof_thing) != ID_KNOWN_TYPE)
            return true;

        switch (kindof_thing)
        {
            case POT_BLOOD:
            case POT_BLOOD_COAGULATED:
                if (ur_herbivorous)
                {
                    if (!suppress_msg)
                        mpr("Sorry, you're a herbivore.");
                    return false;
                }
                return true;
             case POT_PORRIDGE:
                if (you.species == SP_VAMPIRE)
                {
                    if (!suppress_msg)
                        mpr("Blech - you need blood!");
                    return false;
                }
                else if (ur_carnivorous)
                {
                    if (!suppress_msg)
                        mpr("Sorry, you're a carnivore.");
                    return false;
                }
             default:
                return true;
        }

    // Other object types are set to return false for now until
    // someone wants to recode the eating code to permit consumption
    // of things other than just food.
    default:
        return false;
    }

    return survey_says;
}

bool chunk_is_poisonous(int chunktype)
{
    return chunktype == CE_POISONOUS || chunktype == CE_POISON_CONTAM;
}

// See if you can follow along here -- except for the amulet of the gourmand
// addition (long missing and requested), what follows is an expansion of how
// chunks were handled in the codebase up to this date ... {dlb}
// Unidentified rPois gear is ignored here
static corpse_effect_type _determine_chunk_effect(corpse_effect_type chunktype,
                                                  bool rotten_chunk)
{
    // Determine the initial effect of eating a particular chunk. {dlb}
    switch (chunktype)
    {
    case CE_ROT:
    case CE_MUTAGEN:
        if (you.species == SP_GHOUL)
            chunktype = CE_CLEAN;
        break;

    case CE_POISONOUS:
        if (player_res_poison(false) > 0)
            chunktype = CE_CLEAN;
        break;

    case CE_POISON_CONTAM:
        if (player_res_poison(false) <= 0)
        {
            chunktype = CE_POISONOUS;
            break;
        }
        else
        {
            chunktype = CE_CONTAMINATED;
            break;
        }

    default:
        break;
    }

    // Determine effects of rotting on base chunk effect {dlb}:
    if (rotten_chunk)
    {
        switch (chunktype)
        {
        case CE_CLEAN:
        case CE_CONTAMINATED:
            chunktype = CE_ROTTEN;
            break;
        default:
            break;
        }
    }

    return chunktype;
}

static bool _consume_corpse(int slot, bool invent)
{
    ASSERT(species_can_eat_corpses(you.species));

    item_def &corpse = (invent ? you.inv[slot]
                               : mitm[slot]);

    ASSERT(corpse.base_type == OBJ_CORPSES);
    ASSERT(corpse.sub_type == CORPSE_BODY);

    if (you.species == SP_VAMPIRE)
    {
        if (!mons_has_blood(corpse.mon_type))
        {
            mpr("There is no blood in this body!");
            return false;
        }

        if (food_is_rotten(corpse))
        {
            mpr("It's not fresh enough.");
            return false;
        }
    }

    // The delay for eating a chunk (mass 1000) is 2
    // Here the base nutrition value equals that of chunks,
    // but the delay should be smaller.
    const int max_chunks = get_max_corpse_chunks(corpse.mon_type);
    int duration = 1 + max_chunks / 3;
    duration = stepdown_value(duration, 6, 6, 12, 12);

    // Get some nutrition right away, in case we're interrupted.
    // (-1 for the starting message.)
    nutrition_per_turn(corpse, -1);

    // The draining delay doesn't have a start action, and we only need
    // the continue/finish messages if it takes longer than 1 turn.
    start_delay(DELAY_EAT_CORPSE, duration, invent, slot);

    return true;
}

static void _heal_from_food(int hp_amt, bool unrot)
{
    if (hp_amt > 0)
        inc_hp(hp_amt);

    if (unrot && player_rotted())
    {
        mpr("You feel more resilient.");
        unrot_hp(1);
    }

    calc_hp();
    calc_mp();
}

int you_max_hunger()
{
    if (you_foodless())
        return HUNGER_DEFAULT;

    // Ghouls can never be full or above.
    if (you.species == SP_GHOUL)
        return hunger_threshold[HS_SATIATED];

    return hunger_threshold[HS_ENGORGED];
}

int you_min_hunger()
{
    // This case shouldn't actually happen.
    if (you_foodless())
        return HUNGER_DEFAULT;

    // Vampires can never starve to death.  Ghouls will just rot much faster.
    if (you.is_undead)
        return 601;

    return 0;
}

void handle_starvation()
{
    if (!you_foodless() && !you.duration[DUR_DEATHS_DOOR]
        && you.hunger <= HUNGER_FAINTING)
    {
        if (!you.cannot_act() && one_chance_in(40))
        {
            mprf(MSGCH_FOOD, "You lose consciousness!");
            stop_running();

            you.increase_duration(DUR_PARALYSIS, 5 + random2(8), 13);
            if (you_worship(GOD_XOM))
                xom_is_stimulated(get_tension() > 0 ? 200 : 100);
        }

        if (you.hunger <= 0)
        {
            mprf(MSGCH_FOOD, "You have starved to death.");
            ouch(INSTANT_DEATH, NON_MONSTER, KILLED_BY_STARVATION);
            if (!you.dead) // if we're still here...
                set_hunger(HUNGER_DEFAULT, true);
        }
    }
}

string hunger_cost_string(const int hunger)
{
    if (you_foodless(true))
        return "N/A";

#ifdef WIZARD
    if (you.wizard)
        return make_stringf("%d", hunger);
#endif

    const int breakpoints[] = { 1, 21, 61, 121, 201, 301, 421 };
    const int numbars = breakpoint_rank(hunger, breakpoints, ARRAYSZ(breakpoints));

    if (numbars > 0)
        return string(numbars, '#') + string(ARRAYSZ(breakpoints) - numbars, '.');
    else
        return "None";
}

// Simulacrum and Sublimation of Blood are handled elsewhere, as they ignore
// chunk edibility.
static int _chunks_needed()
{
    if (you.form == TRAN_LICH)
        return 1; // possibly low success rate, so don't drop everything

    int gut = hunger_threshold[HS_HUNGRY + player_likes_chunks()];
    int hunger = gut - you.hunger;

    int appetite = player_hunger_rate(false);
    hunger += appetite * FRESHEST_CORPSE * 2;

    bool channeling = you_worship(GOD_SIF_MUNA);
    for (int i = 0; i < ENDOFPACK; i++)
    {
        if (you.inv[i].defined()
            && you.inv[i].base_type == OBJ_STAVES
            && you.inv[i].sub_type == STAFF_ENERGY)
        {
            channeling = true;
        }
    }

    if (channeling)
        hunger += 2000; // a massive food sink!

    int biggest_spell = 0;
    for (int i = 0; i < MAX_KNOWN_SPELLS; i++)
        if (spell_hunger(you.spells[i]) > biggest_spell)
            biggest_spell = spell_hunger(you.spells[i]);
    hunger += biggest_spell;

    int needed = 1 + max(0, hunger / 666);
    dprf("want to keep %d chunks", needed);

    return needed;
}

static bool _compare_second(const pair<int, int> &a, const pair<int, int> &b)
{
    return a.second < b.second;
}

/**
 * Try to free an inventory slot by dropping a stack of chunks.
 * @return  True if a stack was dropped, false otherwise.
**/
bool drop_spoiled_chunks()
{
    if (Options.auto_drop_chunks == ADC_NEVER)
        return false;

    bool wants_any = you.has_spell(SPELL_SUBLIMATION_OF_BLOOD);
    int nchunks = 0;
    vector<pair<int, int> > chunk_slots;
    for (int slot = 0; slot < ENDOFPACK; slot++)
    {
        item_def &item(you.inv[slot]);

        if (!item.defined()
            || item.base_type != OBJ_FOOD
            || item.sub_type != FOOD_CHUNK)
        {
            continue;
        }

        bool rotten = food_is_rotten(item);
        if (rotten && !you.mutation[MUT_SAPROVOROUS] && !wants_any)
            return drop_item(slot, item.quantity);

        corpse_effect_type ce = _determine_chunk_effect(mons_corpse_effect(
                                                            item.mon_type),
                                                        rotten);
        if (ce == CE_MUTAGEN || ce == CE_ROT)
            continue; // no nutrition from those

        // We assume that carrying poisonous chunks means you can swap rPois in.
        int badness = _corpse_badness(ce, item, wants_any);
        nchunks += item.quantity;
        chunk_slots.push_back(pair<int,int>(slot, badness));
    }

    // No rotten ones to drop, and we're not allowed to drop others.
    if (Options.auto_drop_chunks == ADC_ROTTEN)
        return false;

    nchunks -= _chunks_needed();
    if (nchunks <= 0)
        return false;

    sort(chunk_slots.begin(), chunk_slots.end(), _compare_second);
    while (!chunk_slots.empty())
    {
        int slot = chunk_slots.back().first;
        chunk_slots.pop_back();

        // A slot was freed up.
        if (nchunks >= you.inv[slot].quantity)
            return drop_item(slot, you.inv[slot].quantity);
    }
    return false;
}
