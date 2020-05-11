// This needs to be re-ordered when TAG_MAJOR_VERSION changes!
static const vector<spell_type> spellbook_templates[] =
{

{   // Book of Minor Magic
    SPELL_SHOCK,
    SPELL_PASSWALL,
    SPELL_CONJURE_FLAME,
    SPELL_ICE_STATUE,
    SPELL_BLINK,
},

#if TAG_MAJOR_VERSION == 34
{   // Book of Conjurations
    SPELL_SANDBLAST,
    SPELL_DISCHARGE,
    SPELL_HAILSTORM,
    SPELL_BATTLESPHERE,
},
#endif

{   // Book of Flames
    SPELL_FOXFIRE,
    SPELL_CONJURE_FLAME,
    SPELL_AMBULATORY_BOMB,
    SPELL_DETONATE,
},

{   // Book of Frost
    SPELL_FREEZE,
    SPELL_HIBERNATION,
    SPELL_OZOCUBUS_ARMOUR,
    SPELL_HAILSTORM,
    SPELL_ENGLACIATION,
},

{   // Book of Summonings
    SPELL_SUMMON_MANA_VIPER,
    SPELL_SUMMON_FOREST,
    SPELL_SUMMON_HYDRA,
},

{   // Book of Fire
    SPELL_STARBURST,
    SPELL_FIERY_DISJUNCTION,
    SPELL_IGNITION,
},

{   // Book of Ice
    SPELL_ICE_FORM,
    SPELL_FREEZING_CLOUD,
    SPELL_WINTERS_EMBRACE,
},

{   // Book of Spatial Translocations
    SPELL_PHASE_BUG,
    SPELL_BECKONING,
    SPELL_BLINK,
    SPELL_GOLUBRIAS_PASSAGE,
},

{   // Book of Enchantments
    SPELL_CAUSE_FEAR,
    SPELL_DEFLECT_MISSILES,
    SPELL_DISCORD,
    SPELL_HASTE,
},

#if TAG_MAJOR_VERSION == 34
{   // Young Poisoner's Handbook
    SPELL_MEPHITIC_CLOUD,
},
#endif

{   // Book of the Tempests
    SPELL_IGNITION,
    SPELL_TORNADO,
    SPELL_SHATTER,
},

{   // Book of Death
    SPELL_AFFLICTION,
    SPELL_EXCRUCIATING_WOUNDS,
    SPELL_DEATH_CHANNEL,
},

{   // Book of Misfortune
    SPELL_CONFUSING_TOUCH,
    SPELL_GORGONS_GAZE,
    SPELL_ENGLACIATION,
    SPELL_VIOLENT_UNRAVELLING,
},

{   // Book of Changes
    SPELL_BEASTLY_APPENDAGE,
    SPELL_PASSWALL,
    SPELL_SPIDER_FORM,
    SPELL_ICE_FORM,
    SPELL_BLADE_HANDS,
},

{   // Book of Transfigurations
    SPELL_STATUE_FORM,
    SPELL_DRAGON_FORM,
    SPELL_DEVOURER_FORM,
},

{   // Fen Folio
    SPELL_SUMMON_FOREST,
    SPELL_LEDAS_LIQUEFACTION,
    SPELL_SUMMON_HYDRA,
},

#if TAG_MAJOR_VERSION > 34
{   // Book of Battle
    SPELL_BEASTLY_APPENDAGE,
    SPELL_SONG_OF_SLAYING,
    SPELL_OZOCUBUS_ARMOUR,
    SPELL_SPECTRAL_WEAPON,
},
#endif
{   // Book of Clouds
    SPELL_CONJURE_FLAME,
    SPELL_MEPHITIC_CLOUD,
    SPELL_FREEZING_CLOUD,
},

{   // Book of Necromancy
    SPELL_ESSENCE_SPRAY,
    SPELL_VAMPIRE_KISS,
    SPELL_ANIMATE_DEAD,
},

{   // Book of Callings
    SPELL_PHASE_BUG,
    SPELL_CALL_IMP,
    SPELL_ICE_STATUE,
    SPELL_SUMMON_LIGHTNING_SPIRE,
},

{   // Book of Maledictions
    SPELL_DISTRACTING_TOUCH,
    SPELL_HIBERNATION,
    SPELL_CONFUSING_TOUCH,
    SPELL_DAZZLING_FLASH,
},

{   // Book of Air
    SPELL_BECKONING,
    SPELL_DISCHARGE,
	SPELL_MEPHITIC_CLOUD,
    SPELL_ELECTRIC_SURGE,
},

{   // Book of the Sky
    SPELL_SUMMON_LIGHTNING_SPIRE,
    SPELL_DEFLECT_MISSILES,
    SPELL_CONJURE_BALL_LIGHTNING,
    SPELL_TORNADO,
},

{   // Book of the Warp
    SPELL_FORCE_QUAKE,
    SPELL_PIERCING_SHOT,
    SPELL_FIERY_DISJUNCTION,
    SPELL_CONTROLLED_BLINK,
},

#if TAG_MAJOR_VERSION == 34
{   // Book of Envenomations
    SPELL_SPIDER_FORM,
},
#endif

{   // Book of Unlife
    SPELL_ANIMATE_DEAD,
    SPELL_EXCRUCIATING_WOUNDS,
    SPELL_CIGOTUVIS_EMBRACE,
    SPELL_DEATH_CHANNEL,
},

#if TAG_MAJOR_VERSION == 34
{   // Book of Control
    SPELL_ENGLACIATION,
},

{   // Book of Battle (replacing Morphology)
    SPELL_BEASTLY_APPENDAGE,
    SPELL_SONG_OF_SLAYING,
    SPELL_OZOCUBUS_ARMOUR,
    SPELL_SPECTRAL_WEAPON,
},
#endif

{   // Book of Geomancy
    SPELL_SANDBLAST,
    SPELL_PASSWALL,
    SPELL_FORCE_QUAKE,
    SPELL_GORGONS_GAZE,
    SPELL_STONE_SHARDS,
},

{   // Book of Earth
    SPELL_LEDAS_LIQUEFACTION,
    SPELL_STATUE_FORM,
    SPELL_IOOD,
    SPELL_SHATTER,
},

#if TAG_MAJOR_VERSION == 34
{   // Book of Wizardry
    SPELL_FORCE_QUAKE,
    SPELL_AFFLICTION,
    SPELL_HASTE,
},
#endif

{   // Book of Power
    SPELL_ELECTRIC_SURGE,
    SPELL_BATTLESPHERE,
    SPELL_STARBURST,
    SPELL_WINTERS_EMBRACE,
    SPELL_IOOD,
},

{   // Book of Cantrips
    SPELL_BEASTLY_APPENDAGE,
    SPELL_DISTRACTING_TOUCH,
    SPELL_PHASE_BUG,
},

{   // Book of Party Tricks
    SPELL_BECKONING,
    SPELL_AMBULATORY_BOMB,
    SPELL_INVISIBILITY,
},

#if TAG_MAJOR_VERSION == 34
{   // Akashic Record
    SPELL_FIERY_DISJUNCTION,
    SPELL_CONTROLLED_BLINK,
},
#endif

{   // Book of Arcane Marksmanship
    SPELL_SONG_OF_SLAYING,
    SPELL_BLINK,
    SPELL_PORTAL_PROJECTILE,
    SPELL_CAUSE_FEAR,
},

{   // Book of the Dragon
    SPELL_CAUSE_FEAR,
    SPELL_STARBURST,
    SPELL_DRAGON_FORM,
},

{   // Book of Burglary
    SPELL_PASSWALL,
    SPELL_GOLUBRIAS_PASSAGE,
    SPELL_STONE_SHARDS,
    SPELL_INVISIBILITY,
},

{   // Book of Dreams
    SPELL_HIBERNATION,
    SPELL_WARP_GRAVITY,
    SPELL_INVISIBILITY,
},

{   // Book of Alchemy
    SPELL_DAZZLING_FLASH,
    SPELL_GORGONS_GAZE,
    SPELL_WARP_GRAVITY,
},

{   // Book of Beasts
    SPELL_ICE_STATUE,
    SPELL_SUMMON_MANA_VIPER,
    SPELL_SUMMON_HYDRA,
},

{   // Book of Annihilations
    SPELL_IOOD,
    SPELL_CHAIN_LIGHTNING,
    SPELL_ABSOLUTE_ZERO,
    SPELL_PYROCLASM,
},

{   // Grand Grimoire
    SPELL_GHOSTLY_LEGION,
    SPELL_ELDRITCH_ICHOR,
},

{   // Necronomicon
    SPELL_DEVOURER_FORM,
    SPELL_INFESTATION,
    SPELL_BORGNJORS_REVIVIFICATION,
    SPELL_DEATHS_DOOR,
},

};

COMPILE_CHECK(ARRAYSZ(spellbook_templates) == 1 + MAX_FIXED_BOOK);
