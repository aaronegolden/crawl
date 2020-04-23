// This needs to be re-ordered when TAG_MAJOR_VERSION changes!
static const vector<spell_type> spellbook_templates[] =
{

{   // Book of Minor Magic
    SPELL_SHOCK,
    SPELL_PASSWALL,
    SPELL_CONJURE_FLAME,
    SPELL_CALL_CANINE_FAMILIAR,
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
    SPELL_SUMMON_FOREST,
    SPELL_SUMMON_MANA_VIPER,
},

{   // Book of Fire
    SPELL_DETONATE,
    SPELL_STARBURST,
    SPELL_RING_OF_FLAMES,
    SPELL_IGNITION,
},

{   // Book of Ice
    SPELL_ICE_FORM,
    SPELL_FREEZING_CLOUD,
    SPELL_OZOCUBUS_REFRIGERATION,
},

{   // Book of Spatial Translocations
    SPELL_BECKONING,
    SPELL_BLINK,
    SPELL_GOLUBRIAS_PASSAGE,
},

{   // Book of Enchantments
    SPELL_CAUSE_FEAR,
    SPELL_VIOLENT_UNRAVELLING,
    SPELL_DEFLECT_MISSILES,
    SPELL_DISCORD,
    SPELL_HASTE,
},

#if TAG_MAJOR_VERSION == 34
{   // Young Poisoner's Handbook
    SPELL_MEPHITIC_CLOUD,
    SPELL_INTOXICATE,
},
#endif

{   // Book of the Tempests
    SPELL_DISCHARGE,
    SPELL_ELECTRIC_SURGE,
    SPELL_IGNITION,
    SPELL_TORNADO,
    SPELL_SHATTER,
},

{   // Book of Death
    SPELL_AGONY,
    SPELL_EXCRUCIATING_WOUNDS,
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
    SPELL_IRRADIATE,
    SPELL_STATUE_FORM,
    SPELL_HYDRA_FORM,
    SPELL_DRAGON_FORM,
},

{   // Fen Folio
    SPELL_SUMMON_FOREST,
    SPELL_LEDAS_LIQUEFACTION,
    SPELL_HYDRA_FORM,
    SPELL_SUMMON_HYDRA,
},

#if TAG_MAJOR_VERSION > 34
{   // Book of Battle
    SPELL_BEASTLY_APPENDAGE,
    SPELL_SONG_OF_SLAYING,
    SPELL_REGENERATION,
    SPELL_OZOCUBUS_ARMOUR,
    SPELL_SPECTRAL_WEAPON,
},
#endif
{   // Book of Clouds
    SPELL_CONJURE_FLAME,
    SPELL_MEPHITIC_CLOUD,
    SPELL_FREEZING_CLOUD,
    SPELL_RING_OF_FLAMES,
},

{   // Book of Necromancy
    SPELL_PAIN,
    SPELL_VAMPIRIC_DRAINING,
    SPELL_ANIMATE_DEAD,
},

{   // Book of Callings
    SPELL_SUMMON_SMALL_MAMMAL,
    SPELL_CALL_IMP,
    SPELL_CALL_CANINE_FAMILIAR,
    SPELL_AMBULATORY_BOMB,
    SPELL_SUMMON_LIGHTNING_SPIRE,
},

{   // Book of Maledictions
    SPELL_DISTRACTING_TOUCH,
    SPELL_HIBERNATION,
    SPELL_CONFUSING_TOUCH,
    SPELL_DAZZLING_FLASH,
},

{   // Book of Air
    SPELL_SHOCK,
    SPELL_SWIFTNESS,
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
    SPELL_DISPERSAL,
    SPELL_CONTROLLED_BLINK,
    SPELL_DISJUNCTION,
},

#if TAG_MAJOR_VERSION == 34
{   // Book of Envenomations
    SPELL_SPIDER_FORM,
    SPELL_INTOXICATE,
},
#endif

{   // Book of Unlife
    SPELL_ANIMATE_DEAD,
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
    SPELL_AGONY,
    SPELL_INVISIBILITY,
    SPELL_SPELLFORGED_SERVITOR,
    SPELL_HASTE,
},
#endif

{   // Book of Power
    SPELL_ELECTRIC_SURGE,
    SPELL_BATTLESPHERE,
    SPELL_STARBURST,
    SPELL_OZOCUBUS_REFRIGERATION,
    SPELL_IOOD,
    SPELL_SPELLFORGED_SERVITOR,
},

{   // Book of Cantrips
    SPELL_BEASTLY_APPENDAGE,
    SPELL_SUMMON_SMALL_MAMMAL,
},

{   // Book of Party Tricks
    SPELL_SUMMON_BUTTERFLIES,
    SPELL_BECKONING,
    SPELL_INTOXICATE,
    SPELL_INVISIBILITY
},

#if TAG_MAJOR_VERSION == 34
{   // Akashic Record
    SPELL_DISPERSAL,
    SPELL_MALIGN_GATEWAY,
    SPELL_DISJUNCTION,
    SPELL_CONTROLLED_BLINK,
},
#endif

{   // Book of Arcane Marksmanship
    SPELL_BLINK,
    SPELL_PORTAL_PROJECTILE,
    SPELL_CAUSE_FEAR,
},

{   // Book of the Dragon
    SPELL_CAUSE_FEAR,
    SPELL_STARBURST,
    SPELL_DRAGON_FORM,
    SPELL_DRAGON_CALL,
},

{   // Book of Burglary
    SPELL_SWIFTNESS,
    SPELL_PASSWALL,
    SPELL_GOLUBRIAS_PASSAGE,
    SPELL_STONE_SHARDS,
    SPELL_INVISIBILITY,
},

{   // Book of Dreams
    SPELL_HIBERNATION,
    SPELL_INVISIBILITY,
},

{   // Book of Alchemy
    SPELL_GORGONS_GAZE,
    SPELL_INTOXICATE,
    SPELL_IRRADIATE,
},

{   // Book of Beasts
    SPELL_SUMMON_BUTTERFLIES,
    SPELL_CALL_CANINE_FAMILIAR,
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
    SPELL_MALIGN_GATEWAY,
    SPELL_SUMMON_HORRIBLE_THINGS,
},

{   // Necronomicon
    SPELL_HAUNT,
    SPELL_INFESTATION,
    SPELL_BORGNJORS_REVIVIFICATION,
    SPELL_DEATHS_DOOR,
},

};

COMPILE_CHECK(ARRAYSZ(spellbook_templates) == 1 + MAX_FIXED_BOOK);
