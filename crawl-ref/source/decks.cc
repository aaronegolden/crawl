/**
 * @file
 * @brief Functions with decks of cards.
**/

#include "AppHdr.h"

#include "decks.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

#include "ability.h"
#include "abyss.h"
#include "act-iter.h"
#include "artefact.h"
#include "attitude-change.h"
#include "cloud.h"
#include "coordit.h"
#include "dactions.h"
#include "database.h"
#include "describe.h"
#include "directn.h"
#include "dungeon.h"
#include "english.h"
#include "evoke.h"
#include "food.h"
#include "ghost.h"
#include "godpassive.h" // passive_t::no_haste
#include "godwrath.h"
#include "invent.h"
#include "itemprop.h"
#include "items.h"
#include "item_use.h"
#include "libutil.h"
#include "macro.h"
#include "message.h"
#include "mon-cast.h"
#include "mon-clone.h"
#include "mon-death.h"
#include "mon-movetarget.h"
#include "mon-place.h"
#include "mon-poly.h"
#include "mon-project.h"
#include "mon-tentacle.h"
#include "mon-util.h"
#include "mutation.h"
#include "nearby-danger.h"
#include "notes.h"
#include "output.h"
#include "player-equip.h"
#include "player-stats.h"
#include "potion.h"
#include "prompt.h"
#include "random.h"
#include "religion.h"
#include "spl-clouds.h"
#include "spl-goditem.h"
#include "spl-miscast.h"
#include "spl-monench.h"
#include "spl-other.h"
#include "spl-selfench.h"
#include "spl-summoning.h"
#include "spl-transloc.h"
#include "spl-wpnench.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"
#include "target.h"
#include "teleport.h"
#include "terrain.h"
#include "tiledef-gui.h"
#include "transform.h"
#include "traps.h"
#include "uncancel.h"
#include "unicode.h"
#include "view.h"
#include "xom.h"

// For information on deck structure, reference the comment near the beginning

typedef map<card_type, int> deck_archetype;

deck_archetype deck_of_escape =
{
    { CARD_TOMB,       5 },
    { CARD_EXILE,      5 },
    { CARD_ELIXIR,     5 },
    { CARD_VELOCITY,   5 },
    { CARD_SQUID,      5 },
    { CARD_FAMINE,     5 },
};

deck_archetype deck_of_destruction =
{
    { CARD_VITRIOL,    5 },
    { CARD_STORM,      5 },
    { CARD_LEECH,      5 },
    { CARD_ORB,        5 },
    { CARD_DEGEN,      5 },
    { CARD_WILD_MAGIC, 5 },
};

deck_archetype deck_of_summoning =
{
    { CARD_ELEMENTS,        5 },
    { CARD_SUMMON_DEMON,    5 },
    { CARD_GARDEN,          5 },
    { CARD_SUMMON_FLYING,   5 },
    { CARD_RANGERS,         5 },
    { CARD_ILLUSION,        5 },
};

deck_archetype deck_of_punishment =
{
    { CARD_WRAITH,     5 },
    { CARD_WRATH,      5 },
    { CARD_SWINE,      5 },
    { CARD_TORMENT,    5 },
};

struct deck_type_data
{
    /// The name of the deck. (Doesn't include "deck of ".)
    string name;
    string flavour;
    /// The list of cards this deck contains.
    const deck_archetype cards;
    int deck_max;
};


static map<deck_type, deck_type_data> all_decks =
{
    { DECK_OF_ESCAPE, {
        "escape", "mainly dealing with various forms of escape.",
        deck_of_escape,
        13,
    } },
    { DECK_OF_DESTRUCTION, {
        "destruction", "most of which hurl death and destruction "
            "at one's foes (or, if unlucky, at oneself).",
        deck_of_destruction,
        26,
    } },
    { DECK_OF_SUMMONING, {
        "summoning", "depicting a range of weird and wonderful creatures.",
        deck_of_summoning,
        13,
    } },
    { DECK_OF_PUNISHMENT, {
        "punishment", "which wreak havoc on the user.", deck_of_punishment,
        0, // Not a user deck
    } },
};

vector<ability_type> deck_ability = {
    ABIL_NEMELEX_DRAW_ESCAPE,
    ABIL_NEMELEX_DRAW_DESTRUCTION,
    ABIL_NEMELEX_DRAW_SUMMONING,
    ABIL_NON_ABILITY,
    ABIL_NEMELEX_DRAW_STACK
};

static const deck_archetype _cards_in_deck(deck_type deck)
{
    deck_type_data *deck_data = map_find(all_decks, deck);

    if (deck_data)
        return deck_data->cards;

#ifdef ASSERTS
    die("No cards found for %u", unsigned(deck));
#endif
    return {};
}

const string stack_contents()
{
    const auto& stack = you.props[NEMELEX_STACK_KEY].get_vector();

    string output = "";
    output += comma_separated_fn(
                reverse_iterator<CrawlVector::const_iterator>(stack.end()),
                reverse_iterator<CrawlVector::const_iterator>(stack.begin()),
              [](const CrawlStoreValue& card) { return card_name((card_type)card.get_int()); });
    if (!stack.empty())
        output += ".";

    return output;
}

string deck_summary()
{
    vector<string> stats;
    for (int i = FIRST_PLAYER_DECK; i <= LAST_PLAYER_DECK; i++)
    {
        int cards = deck_cards((deck_type) i);
        const deck_type_data *deck_data = map_find(all_decks, (deck_type) i);
        const string name = deck_data ? deck_data->name : "bugginess";
        if (cards)
        {
            stats.push_back(make_stringf("%d %s card%s", cards,
               name.c_str(), cards == 1 ? "" : "s"));
        }
    }
    return comma_separated_line(stats.begin(), stats.end());
}

const string stack_top()
{
    const auto& stack = you.props[NEMELEX_STACK_KEY].get_vector();
    if (stack.empty())
        return "none";
    else
        return card_name((card_type) stack[stack.size() - 1].get_int());
}

const string deck_contents(deck_type deck)
{
    if (deck == DECK_STACK)
        return "Remaining cards: " + stack_contents();

    string output = "It may contain the following cards: ";

    // This way of doing things is intended to prevent a card
    // that appears in multiple subdecks from showing up twice in the
    // output.
    set<card_type> cards;
    const deck_archetype &pdeck =_cards_in_deck(deck);
    for (const auto& cww : pdeck)
        cards.insert(cww.first);

    output += comma_separated_fn(cards.begin(), cards.end(), card_name);
    output += ".";

    return output;
}

const string deck_flavour(deck_type deck)
{
    if (deck == DECK_STACK)
        return "set aside for later.";

    deck_type_data* deck_data = map_find(all_decks, deck);

    if (deck_data)
        return deck_data->flavour;

    return "";
}

const char* card_name(card_type card)
{
    switch (card)
    {
    case CARD_VELOCITY:        return "Velocity";
    case CARD_EXILE:           return "Exile";
    case CARD_ELIXIR:          return "the Elixir";
    case CARD_STAIRS:          return "the Stairs";
    case CARD_TOMB:            return "the Tomb";
    case CARD_WILD_MAGIC:      return "Wild Magic";
    case CARD_ELEMENTS:        return "the Elements";
    case CARD_SUMMON_DEMON:    return "the Pentagram";
    case CARD_GARDEN:          return "the Garden";
    case CARD_SUMMON_FLYING:   return "Foxfire";
    case CARD_RANGERS:         return "the Rangers";
    case CARD_SHAFT:           return "the Shaft";
    case CARD_VITRIOL:         return "Vitriol";
    case CARD_SQUID:           return "the Squid";
    case CARD_STORM:           return "the Storm";
    case CARD_LEECH:           return "the Leech";
    case CARD_TORMENT:         return "Torment";
    case CARD_WRATH:           return "Wrath";
    case CARD_WRAITH:          return "the Wraith";
    case CARD_SWINE:           return "the Swine";
    case CARD_ORB:             return "the Orb";
    case CARD_ILLUSION:        return "the Illusion";
    case CARD_DEGEN:           return "Degeneration";
    case CARD_FAMINE:          return "Famine";

    case NUM_CARDS:            return "a buggy card";
    default: return "a very buggy card";
    }
    return "a very buggy card";
}

card_type name_to_card(string name)
{
    for (int i = 0; i < NUM_CARDS; i++)
    {
        if (card_name(static_cast<card_type>(i)) == name)
            return static_cast<card_type>(i);
    }
    return NUM_CARDS;
}

bool gift_cards()
{
    const int deal = random_range(MIN_GIFT_CARDS, MAX_GIFT_CARDS);
    bool dealt_cards = false;

    for (int i = 0; i < deal; i++)
    {
        deck_type choice = random_choose_weighted(
                                        3, DECK_OF_DESTRUCTION,
                                        1, DECK_OF_SUMMONING,
                                        1, DECK_OF_ESCAPE);
        if (you.props[deck_name(choice)].get_int() < all_decks[choice].deck_max)
        {
            you.props[deck_name(choice)]++;
            dealt_cards = true;
        }
    }

    return dealt_cards;
}

void reset_cards()
{
    for (int i = 0; i <= LAST_PLAYER_DECK; i++)
        you.props[deck_name((deck_type) i)] = 0;
}

static card_type _random_card(deck_type deck)
{
    const deck_archetype &pdeck = _cards_in_deck(deck);
    return *random_choose_weighted(pdeck);
}

static char _deck_hotkey(deck_type deck)
{
    return get_talent(deck_ability[deck], false).hotkey;
}

static deck_type _choose_deck(const string title = "Draw")
{
    ToggleableMenu deck_menu(MF_SINGLESELECT
            | MF_NOWRAP | MF_TOGGLE_ACTION | MF_ALWAYS_SHOW_MORE);
#ifdef USE_TILE_LOCAL
    {
        ToggleableMenuEntry* me =
            new ToggleableMenuEntry(make_stringf("%s which deck?        "
                                    "Cards available", title.c_str()),
                                    "Describe which deck?    "
                                    "Cards available",
                                    MEL_TITLE);
        deck_menu.add_entry(me);
    }
#else
    deck_menu.set_title(new ToggleableMenuEntry(make_stringf("%s which deck?        "
                                    "Cards available", title.c_str()),
                                    "Describe which deck?    "
                                    "Cards available",
                                    MEL_TITLE));
#endif
    deck_menu.set_tag("deck");
    deck_menu.add_toggle_key('!');
    deck_menu.add_toggle_key('?');
    deck_menu.menu_action = Menu::ACT_EXECUTE;

    deck_menu.set_more(formatted_string::parse_string(
                       "Press '<w>!</w>' or '<w>?</w>' to toggle "
                       "between deck selection and description."));

    int numbers[NUM_DECKS];

    for (int i = FIRST_PLAYER_DECK; i <= LAST_PLAYER_DECK; i++)
    {
        ToggleableMenuEntry* me =
            new ToggleableMenuEntry(deck_status(static_cast<deck_type>(i)),
                    deck_status(static_cast<deck_type>(i)),
                    MEL_ITEM, 1, _deck_hotkey(static_cast<deck_type>(i)));
        numbers[i] = i;
        me->data = &numbers[i];
        if (!deck_cards((deck_type)i))
            me->colour = COL_USELESS;

#ifdef USE_TILE
        me->add_tile(tile_def(TILEG_NEMELEX_DECK + i - FIRST_PLAYER_DECK + 1, TEX_GUI));
#endif
        deck_menu.add_entry(me);
    }

    int ret = NUM_DECKS;
    deck_menu.on_single_selection = [&deck_menu, &ret](const MenuEntry& sel)
    {
        ASSERT(sel.hotkeys.size() == 1);
        int selected = *(static_cast<int*>(sel.data));

        if (deck_menu.menu_action == Menu::ACT_EXAMINE)
            describe_deck((deck_type) selected);
        else
            ret = *(static_cast<int*>(sel.data));
        return deck_menu.menu_action == Menu::ACT_EXAMINE;
    };
    deck_menu.show(false);
    if (!crawl_state.doing_prev_cmd_again)
        redraw_screen();
    return (deck_type) ret;
}

static string _empty_deck_msg()
{
    string message = random_choose("disappears without a trace.",
        "glows slightly and disappears.",
        "glows with a rainbow of weird colours and disappears.");
    return "The deck of cards " + message;
}

static void _evoke_deck(deck_type deck, bool dealt = false)
{
    ASSERT(deck_cards(deck) > 0);

    mprf("You %s a card...", dealt ? "deal" : "draw");

    if (deck == DECK_STACK)
    {
        auto& stack = you.props[NEMELEX_STACK_KEY].get_vector();
        card_type card = (card_type) stack[stack.size() - 1].get_int();
        stack.pop_back();
        card_effect(card, dealt);
    }
    else
    {
        --you.props[deck_name(deck)];
        card_effect(_random_card(deck), dealt);
    }

    if (!deck_cards(deck))
        mpr(_empty_deck_msg());
}

// Draw one card from a deck, prompting the user for a choice
bool deck_draw(deck_type deck)
{
    if (!deck_cards(deck))
    {
        mpr("That deck is empty!");
        return false;
    }

    _evoke_deck(deck);
    return true;
}

bool deck_stack()
{
    int total_cards = 0;

    for (int i = FIRST_PLAYER_DECK; i <= LAST_PLAYER_DECK; ++i)
        total_cards += deck_cards((deck_type) i);

    if (deck_cards(DECK_STACK) && !yesno("Replace your current stack?",
                                          false, 0))
    {
        return false;
    }

    if (!total_cards)
    {
        mpr("You are out of cards!");
        return false;
    }

    if (total_cards < 5 && !yesno("You have fewer than five cards, "
                                  "stack them anyway?", false, 0))
    {
        canned_msg(MSG_OK);
        return false;
    }

    you.props[NEMELEX_STACK_KEY].get_vector().clear();
    run_uncancel(UNC_STACK_FIVE, min(total_cards, 5));
    return true;
}

class StackFiveMenu : public ToggleableMenu
{
    virtual bool process_key(int keyin) override;
    CrawlVector& draws;
public:
    StackFiveMenu(CrawlVector& d)
        : ToggleableMenu(MF_NOSELECT | MF_UNCANCEL | MF_ALWAYS_SHOW_MORE), draws(d) {};
};

static int _describe_cards(CrawlVector& cards)
{
    ASSERT(!cards.empty());

    string description;
    description += ("\n Card            Effect");

    bool seen[NUM_CARDS] = {0};
    for (auto& val : cards)
    {
        card_type card = (card_type) val.get_int();

        if (seen[card])
            continue;
        seen[card] = true;
        
        description += ("\n");

        string name = card_name(card);
        string desc = getLongDescription(name + " card");
        if (desc.empty())
            desc = "No description found.\n";
        string decks = which_decks(card);

        name = uppercase_first(name);
        desc = desc + decks;

        description += (name + "     " + desc + "\n");
    }
    
    formatted_scroller desc_fs;
    int flags = MF_NOSELECT | MF_ALWAYS_SHOW_MORE;
    desc_fs.set_flags(flags, false);
    desc_fs.set_more();
    desc_fs.add_text(description, false, get_number_of_cols());
    
    while (true)
    {
        desc_fs.show();
        int keyin = desc_fs.getkey();
        if(keyin == ESCAPE)
        {
            clrscr();
            return keyin;
        }
    }
    return 0;   
}

bool StackFiveMenu::process_key(int keyin)
{
    if (keyin == CK_ENTER)
    {
        return false;
    }
    else if (keyin == '?' || keyin == '!')
    {
        _describe_cards(draws);
        show();
        return false;
    }
    else if (keyin >= '1' && keyin <= '0' + static_cast<int>(draws.size()))
    {
        const unsigned int i = keyin - '1';
        for (unsigned int j = 0; j < items.size(); j++)
            if (items[j]->selected())
            {
                swap(draws[i], draws[j]);
                swap(items[i]->text, items[j]->text);
                items[j]->colour = LIGHTGREY;
                select_item_index(i, 0, false); // this also updates the item
                select_item_index(j, 0, false);
                return true;
            }
        items[i]->colour = WHITE;
        select_item_index(i, 1, false);
    }
    else
        Menu::process_key(keyin);
    return true;
}

// Draw the top four cards of an deck and play them all.
bool deck_deal()
{
    deck_type choice = _choose_deck("Deal");

    if (choice == NUM_DECKS)
        return false;

    int num_cards = deck_cards(choice);

    if (!num_cards)
    {
        mpr("That deck is empty!");
        return false;
    }

    const int num_to_deal = min(num_cards, 4);

    for (int i = 0; i < num_to_deal; ++i)
        _evoke_deck(choice, true);

    return true;
}

string which_decks(card_type card)
{
    vector<string> decks;
    string output = "\n";
    bool punishment = false;
    for (auto &deck_data : all_decks)
    {
        if (!deck_data.second.cards.count(card))
            continue;

        if (deck_data.first == DECK_OF_PUNISHMENT)
            punishment = true;
        else
            decks.push_back(deck_data.second.name);
    }

    if (!decks.empty())
    {
        output += "It is found in decks of "
               +  comma_separated_line(decks.begin(), decks.end());
        if (punishment)
            output += ", or in Nemelex Xobeh's deck of punishment";
        output += ".";
    }
    else if (punishment)
    {
        output += "It is only found in Nemelex Xobeh's deck of "
                  "punishment.";
    }
    else
        output += "It is normally not part of any deck.";

    return output;
}

string deck_status(deck_type deck)
{
    const string name = deck_name(deck);
    const int cards   = deck_cards(deck);

    ostringstream desc;

    desc << chop_string(deck_name(deck), 24)
         << to_string(cards);

    return trimmed_string(desc.str());
}

string deck_description(deck_type deck)
{
    ostringstream desc;

    desc << "A deck of magical cards, ";
    desc << deck_flavour(deck) << "\n\n";
    desc << deck_contents(deck) << "\n";

    if (deck != DECK_STACK)
    {
        const int cards = deck_cards(deck);
        desc << "\n";

        if (cards > 1)
            desc << make_stringf("It currently has %d cards ", cards);
        else if (cards == 1)
            desc << "It currently has 1 card ";
        else
            desc << "It is currently empty ";

        desc << make_stringf("and can contain up to %d cards.",
                             all_decks[deck].deck_max);
        desc << "\n";
    }

    return desc.str();
}

static void _draw_stack(int to_stack)
{
    ToggleableMenu deck_menu(MF_SINGLESELECT | MF_UNCANCEL
            | MF_NOWRAP | MF_TOGGLE_ACTION | MF_ALWAYS_SHOW_MORE);
    {
        ToggleableMenuEntry* me =
            new ToggleableMenuEntry("Draw which deck?        "
                                    "Cards available",
                                    "Describe which deck?    "
                                    "Cards available",
                                    MEL_TITLE);
        deck_menu.set_title(me);
    }
    deck_menu.set_tag("deck");
    deck_menu.add_toggle_key('!');
    deck_menu.add_toggle_key('?');
    deck_menu.menu_action = Menu::ACT_EXECUTE;

    auto& stack = you.props[NEMELEX_STACK_KEY].get_vector();

    if (!stack.empty())
    {
            string status = "Drawn so far: " + stack_contents();
            deck_menu.set_more(formatted_string::parse_string(
                       status + "\n" +
                       "Press '<w>!</w>' or '<w>?</w>' to toggle "
                       "between deck selection and description."));
    }
    else
    {
        deck_menu.set_more(formatted_string::parse_string(
                           "Press '<w>!</w>' or '<w>?</w>' to toggle "
                           "between deck selection and description."));
    }

    int numbers[NUM_DECKS];

    for (int i = FIRST_PLAYER_DECK; i <= LAST_PLAYER_DECK; i++)
    {
        ToggleableMenuEntry* me =
            new ToggleableMenuEntry(deck_status((deck_type)i),
                    deck_status((deck_type)i),
                    MEL_ITEM, 1, _deck_hotkey((deck_type)i));
        numbers[i] = i;
        me->data = &numbers[i];
        if (!deck_cards((deck_type)i))
            me->colour = COL_USELESS;

#ifdef USE_TILE
        me->add_tile(tile_def(TILEG_NEMELEX_DECK + i - FIRST_PLAYER_DECK + 1, TEX_GUI));
#endif
        deck_menu.add_entry(me);
    }
    deck_menu.on_single_selection = [&deck_menu, &stack, to_stack](const MenuEntry& sel)
    {
        ASSERT(sel.hotkeys.size() == 1);
        deck_type selected = (deck_type) *(static_cast<int*>(sel.data));
        // Need non-const access to the selection.
        ToggleableMenuEntry* me =
            static_cast<ToggleableMenuEntry*>(deck_menu.selected_entries()[0]);

        if (deck_menu.menu_action == Menu::ACT_EXAMINE)
        {
            describe_deck(selected);
        }
        else if (you.props[deck_name(selected)].get_int() > 0)
        {
            you.props[deck_name(selected)]--;
            me->text = deck_status(selected);
            me->alt_text = deck_status(selected);

            card_type draw = _random_card(selected);
            stack.push_back(draw);
            string status = "Drawn so far: " + stack_contents();
            deck_menu.set_more(formatted_string::parse_string(
                       status + "\n" +
                       "Press '<w>!</w>' or '<w>?</w>' to toggle "
                       "between deck selection and description."));
        }
        return stack.size() < to_stack
               || deck_menu.menu_action == Menu::ACT_EXAMINE;
    };
    deck_menu.show(false);
}

bool stack_five(int to_stack)
{
    auto& stack = you.props[NEMELEX_STACK_KEY].get_vector();

    while (stack.size() < to_stack)
    {
        if (crawl_state.seen_hups)
            return false;

        _draw_stack(to_stack);
    }

    StackFiveMenu menu(stack);
    MenuEntry *const title = new MenuEntry("Select two cards to swap them:", MEL_TITLE);
    menu.set_title(title);
    menu.add_toggle_key('?');
    menu.menu_action = Menu::ACT_EXECUTE;
    for (unsigned int i = 0; i < stack.size(); i++)
    {
        ToggleableMenuEntry * const entry =
            new ToggleableMenuEntry(card_name((card_type)stack[i].get_int()),
            card_name((card_type)stack[i].get_int()),
                          MEL_ITEM, 1, '1'+i);
#ifdef USE_TILE
        entry->add_tile(tile_def(TILEG_NEMELEX_CARD, TEX_GUI));
#endif
        string name = card_name((card_type)stack[i].get_int());
        string desc = getLongDescription(name + " card");
        entry->alt_text = desc;
        menu.add_entry(entry);
    }
    menu.set_more(formatted_string::parse_string(
                "<lightgrey>Press <w>?</w> for the card descriptions"
                " or <w>Enter</w> to accept."));
    menu.show();

    if (crawl_state.seen_hups)
        return false;
    else
    {
        std::reverse(stack.begin(), stack.end());
        return true;
    }
}

// Draw the next three cards, discard two and pick one.
bool deck_triple_draw()
{
    deck_type choice = _choose_deck();

    if (choice == NUM_DECKS)
        return false;

    int num_cards = deck_cards(choice);

    if (!num_cards)
    {
        mpr("That deck is empty!");
        return false;
    }

    if (num_cards < 3 && !yesno("There's fewer than three cards, "
                                "still triple draw?", false, 0))
    {
        canned_msg(MSG_OK);
        return false;
    }

    if (num_cards == 1)
    {
        // Only one card to draw, so just draw it.
        mpr("There's only one card left!");
        _evoke_deck(choice);
        return true;
    }

    const int num_to_draw = min(num_cards, 3);

    you.props[deck_name(choice)] = deck_cards(choice) - num_to_draw;

    auto& draw = you.props[NEMELEX_TRIPLE_DRAW_KEY].get_vector();
    draw.clear();

    for (int i = 0; i < num_to_draw; ++i)
        draw.push_back(_random_card(choice));

    run_uncancel(UNC_DRAW_THREE, 0);
    return true;
}

bool draw_three()
{
    auto& draws = you.props[NEMELEX_TRIPLE_DRAW_KEY].get_vector();

    int selected = -1;
    bool need_prompt_redraw = true;
    while (true)
    {
        if (need_prompt_redraw)
        {
            mpr("You draw... (choose one card, ? for their descriptions)");
            for (int i = 0; i < draws.size(); ++i)
            {
                msg::streams(MSGCH_PROMPT)
                    << msg::nocap << (static_cast<char>(i + 'a')) << " - "
                    << card_name((card_type)draws[i].get_int()) << endl;
            }
            need_prompt_redraw = false;
        }
        const int keyin = toalower(get_ch());

        if (crawl_state.seen_hups)
            return false;

        if (keyin == '?')
        {
            _describe_cards(draws);
            redraw_screen();
            need_prompt_redraw = true;
        }
        else if (keyin >= 'a' && keyin < 'a' + draws.size())
        {
            selected = keyin - 'a';
            break;
        }
        else
            canned_msg(MSG_HUH);
    }

    card_effect((card_type) draws[selected].get_int());

    return true;
}

// This is Nemelex retribution. If deal is true, use the word "deal"
// rather than "draw" (for the Deal Four out-of-cards situation).
void draw_from_deck_of_punishment(bool deal)
{
    uint8_t flags = CFLAG_PUNISHMENT;
    if (deal)
        flags |= CFLAG_DEALT;
    card_type card = _random_card(DECK_OF_PUNISHMENT);

    mprf("You %s a card...", deal ? "deal" : "draw");
    card_effect(card, flags);
}

// Actual card implementations follow.

static void _velocity_card(int power)
{
    cast_swiftness(div_rand_round(power, 100));
    int haste_turns = div_rand_round(random2(power + 1), 100) - random2(51);
    
    if(haste_turns > 0)
        you.increase_duration(DUR_HASTE, haste_turns);
}

static void _exile_card(int power)
{
    if (player_in_branch(BRANCH_ABYSS))
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return;
    }
    
    int pow = div_rand_round(power, 20);
    
    // same stepdowns as for spellpower
    pow = stepdown_value(pow, 50, 50, 150, 200);

    mprf("A great otherworldly force beckons your foes!");
    
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
    {
        if (mi->wont_attack() && mons_is_threatening(**mi))
            continue;
        
        if (mi->check_res_magic(pow) < 0)
                mi->banish(&you);    
    }
}

static void _shaft_card(int power)
{
    canned_msg(MSG_NOTHING_HAPPENS);
}

static int stair_draw_count = 0;

// This does not describe an actual card. Instead, it only exists to test
// the stair movement effect in wizard mode ("&c stairs").
static void _stairs_card(int /*power*/)
{
    you.duration[DUR_REPEL_STAIRS_MOVE]  = 0;
    you.duration[DUR_REPEL_STAIRS_CLIMB] = 0;

    if (feat_stair_direction(grd(you.pos())) == CMD_NO_CMD)
        you.duration[DUR_REPEL_STAIRS_MOVE]  = 1000;
    else
        you.duration[DUR_REPEL_STAIRS_CLIMB] =  500; // more annoying

    vector<coord_def> stairs_avail;

    for (radius_iterator ri(you.pos(), LOS_DEFAULT, true); ri; ++ri)
    {
        dungeon_feature_type feat = grd(*ri);
        if (feat_stair_direction(feat) != CMD_NO_CMD
            && feat != DNGN_ENTER_SHOP)
        {
            stairs_avail.push_back(*ri);
        }
    }

    if (stairs_avail.empty())
    {
        mpr("No stairs available to move.");
        return;
    }

    shuffle_array(stairs_avail);

    for (coord_def stair : stairs_avail)
        move_stair(stair, stair_draw_count % 2, false);

    stair_draw_count++;
}

static void _leech_card(int power, bool dealt = false)
{
    mprf("You have %s the Leech.", dealt ? "dealt" : "drawn");
    targetter_smite tgt(&you, LOS_RADIUS, 0, 0);
    monster* mon;
    
    while (true)
    {
        direction_chooser_args args;
        args.mode = TARG_HOSTILE;
        args.needs_path = false;
        args.top_prompt = "Aiming: <white>the Leech</white>";
        args.self = CONFIRM_CANCEL;
        args.hitfunc = &tgt;
        dist beam;
        direction(beam, args);
        
        monster* mons = monster_at(beam.target);
        
        if (!mons)
        {
            mpr("There's nothing there!");
            continue;
        }
        if (mons->res_negative_energy() >= 3)
        {
            mpr("You can't drain that!");
            continue;
        }
        if (!you.see_cell_no_trans(beam.target))
        {
            mpr("You can't see that place.");
            continue;
        }
        mon = mons;
        break;
    }
    
    int dam = 2 + random2(7) + div_rand_round(random2(power), 175);
    
    mon->hurt(&you, dam);
    
    if (!you.duration[DUR_DEATHS_DOOR])
    {
        mpr("You feel life coursing into your body.");
        inc_hp(div_rand_round(dam, 2));
    }
}

static void _damaging_card(card_type card, int power,
                           bool dealt = false)
{
    const char *participle = dealt ? "dealt" : "drawn";

    bool done_prompt = false;
    string prompt = make_stringf("You have %s %s.", participle,
                                 card_name(card));

    dist target;
    zap_type ztype = ZAP_DEBUGGING_RAY;
    
    switch (card)
    {
    case CARD_VITRIOL:
    {
        done_prompt = true;
        mpr(prompt);
        mpr("You radiate a wave of entropy!");
        apply_visible_monsters([](monster& mons)
        {
            return !mons.wont_attack()
                    && mons_is_threatening(mons)
                    && mons.corrode_equipment();
        });
        
        ztype = ZAP_CORROSIVE_BOLT;
        break;
    }

    case CARD_ORB:
        ztype = ZAP_IOOD;
        break;
		
    case CARD_STORM:
    {
        wind_blast(&you, 28 + div_rand_round(power, 50), coord_def(), true);
        coord_def pos = you.pos();
        vector<coord_def> targs;
        for (radius_iterator ri(pos, LOS_NO_TRANS); ri; ri++)
        {
            if((pos - *ri).rdist() < 4 || !you.see_cell_no_trans(*ri))
                continue;
            
            targs.emplace_back(*ri);
        }
        
        if (targs.size() == 0)
        {
            mprf("The air crackles with electricity for a moment.");
            return;
        }
        
        vector<coord_def> targets;
        targets.emplace_back(targs[random2(targs.size())]);
        
        int num_additional = random2(12);
        
        while(num_additional > 0)
        {
            num_additional--;
            int x = random_range(4, LOS_RADIUS) * (coinflip() ? -1 : 1);
            int y = random_range(4, LOS_RADIUS) * (coinflip() ? -1 : 1);
            coord_def t = pos + coord_def(x,y);
            
            if(in_bounds(t) && you.see_cell_no_trans(t) && !cell_is_solid(t))
            {
                targets.emplace_back(t);
            }    
        }
        
        bolt beam;
        beam.name           = "orb of electricity";
        beam.flavour        = BEAM_ELECTRICITY;
        beam.colour         = LIGHTBLUE;
        beam.ex_size        = 2;
        beam.source_id      = MID_PLAYER;
        beam.thrower        = KILL_YOU;
        beam.is_explosion   = true;
        beam.hit = AUTOMATIC_HIT;

        for (auto & coord : targets)
        {
            
            beam.damage = calc_dice(1, 13 + div_rand_round(power * 4, 250));
            beam.source = coord;
            beam.target = coord;
            beam.explode();
        }
        
        return;
    }

    default:
        break;
    }

    bolt beam;
    beam.range = LOS_RADIUS;

    direction_chooser_args args;
    args.mode = TARG_HOSTILE;
    if (!done_prompt)
        args.top_prompt = prompt;
    if (spell_direction(target, beam, &args)
        && player_tracer(ZAP_DEBUGGING_RAY, power/6, beam))
    {
        if (you.confused())
        {
            target.confusion_fuzz();
            beam.set_target(target);
        }

        if (ztype == ZAP_IOOD)
        {
            cast_iood_burst(15 + div_rand_round(power, 54), coord_def(-1, -1));
        }
        else if (ztype == ZAP_CORROSIVE_BOLT)
        {
            zapping(ztype, div_rand_round(power, 50), beam);
        }
        else
            zapping(ztype, power/6, beam);
    }
}

static void _elixir_card(int power)
{
    int dur = 1 + div_rand_round(random2(power + 1), 1000);
    
    you.set_duration(DUR_ELIXIR_HEALTH, dur);
    you.set_duration(DUR_ELIXIR_MAGIC, dur);
}

// Special case for *your* god, maybe?
static void _godly_wrath()
{
    for (int tries = 0; tries < 100; tries++)
    {
        god_type god = random_god();

        // Don't recursively make player draw from the Deck of Punishment.
        if (god != GOD_NEMELEX_XOBEH && divine_retribution(god))
            return; // Stop once we find a god willing to punish the player.
    }

    mpr("You somehow manage to escape divine attention...");
}

static void _summon_demon_card(int power)
{
    int tier = 5 - div_rand_round(random2avg(power + 1, 2), 1000);
    monster_type dct;
    
    if (tier < 1)
        dct = MONS_PANDEMONIUM_LORD;
    else
        dct = random_demon_by_tier(tier);

    if (!create_monster(mgen_data(dct, BEH_FRIENDLY,
                                  you.pos(), MHITYOU, MG_AUTOFOE)
                        .set_summoned(&you, 3, 0), false))
    {
        mpr("You see a puff of smoke, but nothing appears.");
    }
}

static void _elements_card(int power)
{
    int num_summons = 1 + div_rand_round(random2(power + 1), 2500);
    monster_type mons_type = random_choose(MONS_AIR_ELEMENTAL,
                                           MONS_FIRE_ELEMENTAL,
                                           MONS_EARTH_ELEMENTAL,
                                           MONS_WATER_ELEMENTAL);

    int successes = 0;
    
    for (int i = 0; i < num_summons; ++i)
    {
        if (create_monster(
                mgen_data(mons_type, BEH_FRIENDLY, you.pos(), MHITYOU,
                            MG_AUTOFOE).set_summoned(&you, 3, 0),
                false))
        {  
            successes++;
        }
    }
    
    if (successes == 0)
    {
        mpr("You see puffs of smoke, but nothing appears.");
    }
}

static void _summon_garden(int power)
{
    int how_many = 4 + div_rand_round(random2(power + 1), 500);
    int successes = 0;
    
    for (int i = 0 ; i < how_many; i++)
    {
        bool upgrade = x_chance_in_y(power, 40000);
        
        if (create_monster(
            mgen_data(upgrade ? MONS_OKLOB_PLANT : MONS_BRIAR_PATCH, 
                        BEH_FRIENDLY, you.pos(), MHITYOU, 
                        MG_AUTOFOE).set_summoned(&you, 3, 0), false))
        {
            successes++;  
        }
    }
    
    if (successes == 0)
        mpr("You see puffs of smoke, but nothing appears.");
}

static void _summon_flying(int power)
{

    const int how_many = 1 + div_rand_round(random2(power + 1), 1000);

    for (int i = 0; i < how_many; ++i)
    {
        create_monster(
            mgen_data(MONS_KILLER_BEE,
                      BEH_FRIENDLY, you.pos(), MHITYOU,
                      MG_AUTOFOE).set_summoned(&you, 3, 0));
    }
}

static void _summon_rangers(int power)
{
    for(int i = 0; i < 2; i++)
    {
        int upgrades = 0;
        if (x_chance_in_y(power, 15000))
            upgrades++;
        if (x_chance_in_y(power, 15000))
            upgrades++;
        
        monster_type type;
       
        switch (upgrades)
        {
            case 0: type = MONS_CENTAUR;
                break;
            case 1: type = MONS_CENTAUR_WARRIOR;
                break;
            default: type = MONS_DEEP_ELF_MASTER_ARCHER;
                break;
        }
        
        create_monster(
            mgen_data(type,
                      BEH_FRIENDLY, you.pos(), MHITYOU,
                      MG_AUTOFOE).set_summoned(&you, 3, 0));
    }
}

static void _cloud_card(int power)
{
    int dur = 1 + div_rand_round(random2(power + 1), 200);
    
    if (you.duration[DUR_SQUID])
        mpr("The smoke pouring from your body grows thicker.");
    else
        mpr("Thick smoke begins to pour from your body!");
    
    you.increase_duration(DUR_SQUID, dur, 100);
}

static void _illusion_card(int power)
{
    monster* mon = get_free_monster();

    if (!mon || monster_at(you.pos()))
        return;

    mon->type = MONS_PLAYER;
    mon->behaviour = BEH_SEEK;
    mon->attitude = ATT_FRIENDLY;
    mon->set_position(you.pos());
    mon->mid = MID_PLAYER;
    mgrd(you.pos()) = mon->mindex();

    mons_summon_illusion_from(mon, (actor *)&you, SPELL_NO_SPELL, power);
    mon->reset();
}

static void _degeneration_card(int power)
{
    int ench_power = zap_ench_power(ZAP_POLYMORPH, div_rand_round(power, 20), false);
    
    if (!apply_visible_monsters([ench_power](monster& mons)
           {
               if (mons.wont_attack() || !mons_is_threatening(mons))
                   return false;
               
                if (!mons.can_polymorph())
                    return false;

               if (mons.check_res_magic(ench_power) > 0)
               {
                   return false;
               }
                   
               monster_polymorph(&mons, RANDOM_MONSTER, PPT_LESS);
               mons.hurt(&you, mons.hit_points / 2);
               
               return true;
           }))
    {
        canned_msg(MSG_NOTHING_HAPPENS);
    }
}

static void _wild_magic_card(int power)
{
    int num_affected = 0;

    for (radius_iterator di(you.pos(), LOS_NO_TRANS); di; ++di)
    {
        monster *mons = monster_at(*di);

        if (!mons || mons->wont_attack() || !mons_is_threatening(*mons))
            continue;
        
        spschool_flag_type type = random_choose(SPTYP_CONJURATION,
                                                    SPTYP_FIRE,
                                                    SPTYP_ICE,
                                                    SPTYP_EARTH,
                                                    SPTYP_AIR,
                                                    SPTYP_POISON);

        MiscastEffect(mons, actor_by_mid(MID_YOU_FAULTLESS),
                        DECK_MISCAST, type,
                        30 + div_rand_round(power, 200), 1,
                        "a card of wild magic");

        num_affected++;
    }

    if (num_affected > 0)
    {
        int mp = 0;

        for (int i = 0; i < num_affected; ++i)
            mp += random2(5);

        mpr("You feel a surge of magic.");
        if (mp && you.magic_points < you.max_magic_points)
        {
            inc_mp(mp);
            canned_msg(MSG_GAIN_MAGIC);
        }
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);
}

static void _torment_card()
{
    if (you.undead_or_demonic())
        holy_word_player(HOLY_WORD_CARD);
    else
        torment_player(&you, TORMENT_CARDS);
}

static void _famine_card(int power)
{
    bool something_happened = false;

    for (radius_iterator di(you.pos(), LOS_NO_TRANS); di; ++di)
    {
        monster *mons = monster_at(*di);

        if (!mons || mons->wont_attack() || !mons_is_threatening(*mons))
            continue;
		 
        if (x_chance_in_y(power, 25000))
        {
			mons->paralyse(&you, 1 + random2(7));
			something_happened = true;
        }
        else
        {
            mons->weaken(&you, 3 + div_rand_round(random2(power + 1), 400));
            something_happened = true;
        }
    }

    if (!something_happened)
        canned_msg(MSG_NOTHING_HAPPENS);
}

// Punishment cards don't have their power adjusted depending on Nemelex piety
// or penance, and are based on experience level instead of evocations skill
// for more appropriate scaling.
static int _card_power()
{
    return 100 + div_rand_round(you.skill(SK_INVOCATIONS, 1100), 3);
}

void card_effect(card_type which_card,
                 bool dealt,
                 bool punishment, bool tell_card)
{
    const char *participle = dealt ? "dealt" : "drawn";
    const int power = _card_power();

    dprf("Card power: %d", power);

    if (tell_card)
    {
        // These card types will usually give this message in the targeting
        // prompt, and the cases where they don't are handled specially.
        if (which_card != CARD_VITRIOL
            && which_card != CARD_LEECH
            && which_card != CARD_ORB)
        {
            mprf("You have %s %s.", participle, card_name(which_card));
        }
    }

    switch (which_card)
    {
    case CARD_VELOCITY:         _velocity_card(power); break;
    case CARD_EXILE:            _exile_card(power); break;
    case CARD_ELIXIR:           _elixir_card(power); break;
    case CARD_STAIRS:           _stairs_card(power); break;
    case CARD_SHAFT:            _shaft_card(power); break;
    case CARD_TOMB:             entomb(div_rand_round(random2avg(power + 1, 3), 20)); break;
    case CARD_WRAITH:           drain_player(power / 4, false, true); break;
    case CARD_WRATH:            _godly_wrath(); break;
    case CARD_SUMMON_DEMON:     _summon_demon_card(power); break;
    case CARD_ELEMENTS:         _elements_card(power); break;
    case CARD_RANGERS:          _summon_rangers(power); break;
    case CARD_GARDEN:           _summon_garden(power); break;
    case CARD_SUMMON_FLYING:    _summon_flying(power); break;
    case CARD_TORMENT:          _torment_card(); break;
    case CARD_SQUID:            _cloud_card(power); break;
    case CARD_ILLUSION:         _illusion_card(power); break;
    case CARD_DEGEN:            _degeneration_card(power); break;
    case CARD_WILD_MAGIC:       _wild_magic_card(power); break;
    case CARD_FAMINE:           _famine_card(power); break;
    case CARD_LEECH:            _leech_card(power); break;

    case CARD_VITRIOL:
    case CARD_ORB:
    case CARD_STORM:
        _damaging_card(which_card, power, dealt);
        break;

    case CARD_SWINE:
        if (transform(5 + power/10 + random2(power/10), TRAN_PIG, true))
            you.transform_uncancellable = true;
        else
            mpr("You feel a momentary urge to oink.");
        break;

    case NUM_CARDS:
    default:
        // The compiler will complain if any card remains unhandled.
        mprf("You have %s a buggy card!", participle);
        break;
    }
}

/**
 * Return the appropriate name for a known deck of the given type.
 *
 * @param sub_type  The type of deck in question.
 * @return          A name, e.g. "deck of destruction".
 *                  If the given type isn't a deck, return "deck of bugginess".
 */
string deck_name(deck_type deck)
{
    if (deck == DECK_STACK)
        return "stacked deck";
    const deck_type_data *deck_data = map_find(all_decks, deck);
    const string name = deck_data ? deck_data->name : "bugginess";
    return "deck of " + name;
}

int deck_cards(deck_type deck)
{
    return deck == DECK_STACK ? you.props[NEMELEX_STACK_KEY].get_vector().size()
                              : you.props[deck_name(deck)].get_int();
}

/**
 * The deck a given ability uses. Asserts if called on an ability that does not
 * use decks.
 *
 * @param abil the ability
 *
 * @return the deck
 */
deck_type ability_deck(ability_type abil)
{
    auto deck = find(deck_ability.begin(), deck_ability.end(), abil);

    ASSERT(deck != deck_ability.end());
    return (deck_type) distance(deck_ability.begin(), deck);
}