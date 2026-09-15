#include "i18n.h"

Language I18n::_currentLang = LANG_EN;

static const char* const STRINGS[LANG_COUNT][STR_MAX_STRINGS] = {
  // =========================================================================
  // LANG_EN (English)
  // =========================================================================
  {
    /* STR_MENU_TITLE           */ "SETUP MENU",
    /* STR_CAT_RACE_CONFIG      */ "1. Race & Kart Setup",
    /* STR_CAT_RPM_ALARM       */ "2. Shift Lights & Alarms",
    /* STR_CAT_TRACK_GPS        */ "3. Track & GPS Database",
    /* STR_CAT_STORAGE_PC       */ "4. Storage & PC Sync",
    /* STR_CAT_DISPLAY_PWM      */ "5. Display & Backlight",
    /* STR_CAT_SYSTEM_LANG      */ "6. System & Language",
    /* STR_CAT_SENSORS_INFO     */ "7. Sensors & Diagnostics",

    /* STR_DRIVE_TYPE           */ "Drive Type",
    /* STR_SHIFT_RPM            */ "Shift RPM",
    /* STR_MAX_RPM              */ "Max RPM Scale",
    /* STR_OVERREV_RPM          */ "Over-Rev Alarm",
    /* STR_WATER_ALARM          */ "Water Temp Alarm",
    /* STR_EGT_ALARM            */ "EGT Temp Alarm",
    /* STR_LED_BRIGHTNESS       */ "RGB LED Brightness",
    /* STR_LED_TEST             */ "Test RGB LEDs",
    /* STR_RPM_DISP_MODE        */ "RPM Bar Display",
    /* STR_RPM_DISP_BOTH        */ "Both (LCD + LEDs)",
    /* STR_RPM_DISP_DISPLAY     */ "Display Only",
    /* STR_RPM_DISP_LEDS        */ "LED Strip Only",
    /* STR_BACKLIGHT_PWM        */ "Backlight (PWM)",
    /* STR_TRACK_SELECT         */ "Select Track",
    /* STR_TRACK_SD_LOAD        */ "Reload Tracks from SD",
    /* STR_USB_MSC_START        */ "Start PC USB Drive",
    /* STR_LANGUAGE             */ "Language",
    /* STR_SHOW_SPEED           */ "Show Speed",
    /* STR_INVERT_DISP          */ "Display Polarity",
    /* STR_UNITS_SPEED          */ "Speed Unit",
    /* STR_UNITS_TEMP           */ "Temp Unit",
    /* STR_RESET_CONFIG         */ "Factory Reset",

    /* STR_DRIVE_DIRECT         */ "Direct Drive (1-Speed)",
    /* STR_DRIVE_CLUTCH         */ "Clutch (OKJ/Rotax/X30)",
    /* STR_DRIVE_SHIFTER        */ "Shifter (KZ 6-Speed)",

    /* STR_LABEL_SPEED          */ "SPEED",
    /* STR_LABEL_RPM            */ "RPM",
    /* STR_LABEL_GEAR           */ "GEAR",
    /* STR_LABEL_LAP            */ "LAP",
    /* STR_LABEL_BEST           */ "BEST",
    /* STR_LABEL_LAST           */ "LAST",
    /* STR_LABEL_DELTA          */ "DELTA",
    /* STR_LABEL_PRED           */ "PRED",
    /* STR_LABEL_SECTOR         */ "SEC",
    /* STR_LABEL_WATER          */ "WATER",
    /* STR_LABEL_EGT            */ "EGT",
    /* STR_LABEL_AIR            */ "AIR",
    /* STR_LABEL_RH             */ "RH",
    /* STR_LABEL_SATS           */ "SATS",
    /* STR_LABEL_BAT            */ "BAT",
    /* STR_LABEL_HRS            */ "ENG HRS",

    /* STR_WARN_SHIFT           */ "** SHIFT **",
    /* STR_WARN_OVERHEAT        */ "WARN: WATER OVERHEAT!",
    /* STR_WARN_EGT             */ "WARN: HIGH EGT!",
    /* STR_WARN_OVERREV         */ "WARN: OVER-REV!",
    /* STR_WARN_LOW_BAT         */ "WARN: LOW BATTERY!",

    /* STR_STATUS_SIMULATION    */ "SIMULATION MODE",
    /* STR_STATUS_WIRELESS_OK   */ "APEX-TRACK LINKED",
    /* STR_STATUS_NO_SIGNAL     */ "NO APEX-TRACK LINK",
    /* STR_STATUS_USB_MSC_ACTIVE*/ "USB MASS STORAGE ACTIVE"
  },

  // =========================================================================
  // LANG_IT (Italiano)
  // =========================================================================
  {
    /* STR_MENU_TITLE           */ "MENU IMPOSTAZIONI",
    /* STR_CAT_RACE_CONFIG      */ "1. Setup Gara & Kart",
    /* STR_CAT_RPM_ALARM       */ "2. Giri & Allarmi LED",
    /* STR_CAT_TRACK_GPS        */ "3. Pista & Database GPS",
    /* STR_CAT_STORAGE_PC       */ "4. Memoria & Sync PC",
    /* STR_CAT_DISPLAY_PWM      */ "5. Display & Retroillum.",
    /* STR_CAT_SYSTEM_LANG      */ "6. Sistema & Lingua",
    /* STR_CAT_SENSORS_INFO     */ "7. Sensori & Diagnostica",

    /* STR_DRIVE_TYPE           */ "Tipo Trasmissione",
    /* STR_SHIFT_RPM            */ "Giri Cambiata",
    /* STR_MAX_RPM              */ "Giri Fondo Scala",
    /* STR_OVERREV_RPM          */ "Allarme Fuorigiri",
    /* STR_WATER_ALARM          */ "Allarme Temp Acqua",
    /* STR_EGT_ALARM            */ "Allarme Temp EGT",
    /* STR_LED_BRIGHTNESS       */ "Luminosita LED RGB",
    /* STR_LED_TEST             */ "Test LED RGB",
    /* STR_RPM_DISP_MODE        */ "Mostra Barra Giri",
    /* STR_RPM_DISP_BOTH        */ "Entrambi (LCD + LED)",
    /* STR_RPM_DISP_DISPLAY     */ "Solo Display",
    /* STR_RPM_DISP_LEDS        */ "Solo Striscia LED",
    /* STR_BACKLIGHT_PWM        */ "Retroilluminazione",
    /* STR_TRACK_SELECT         */ "Seleziona Circuito",
    /* STR_TRACK_SD_LOAD        */ "Ricarica Piste da SD",
    /* STR_USB_MSC_START        */ "Avvia Storage USB PC",
    /* STR_LANGUAGE             */ "Lingua",
    /* STR_SHOW_SPEED           */ "Mostra Velocita",
    /* STR_INVERT_DISP          */ "Polarita Display",
    /* STR_UNITS_SPEED          */ "Unita Velocita",
    /* STR_UNITS_TEMP           */ "Unita Temperatura",
    /* STR_RESET_CONFIG         */ "Ripristino Fabbrica",

    /* STR_DRIVE_DIRECT         */ "Presa Diretta (1-Marcia)",
    /* STR_DRIVE_CLUTCH         */ "Frizione (OKJ/Rotax/X30)",
    /* STR_DRIVE_SHIFTER        */ "KZ Cambio (6 Marce)",

    /* STR_LABEL_SPEED          */ "VEL",
    /* STR_LABEL_RPM            */ "GIRI",
    /* STR_LABEL_GEAR           */ "MARC",
    /* STR_LABEL_LAP            */ "GIRO",
    /* STR_LABEL_BEST           */ "BEST",
    /* STR_LABEL_LAST           */ "ULT",
    /* STR_LABEL_DELTA          */ "DELTA",
    /* STR_LABEL_PRED           */ "PRED",
    /* STR_LABEL_SECTOR         */ "SETT",
    /* STR_LABEL_WATER          */ "ACQUA",
    /* STR_LABEL_EGT            */ "EGT",
    /* STR_LABEL_AIR            */ "ARIA",
    /* STR_LABEL_RH             */ "UR",
    /* STR_LABEL_SATS           */ "SAT",
    /* STR_LABEL_BAT            */ "BAT",
    /* STR_LABEL_HRS            */ "ORE MOT",

    /* STR_WARN_SHIFT           */ "** CAMBIO **",
    /* STR_WARN_OVERHEAT        */ "ATTENZIONE: SURRISC. ACQUA!",
    /* STR_WARN_EGT             */ "ATTENZIONE: EGT ELEVATO!",
    /* STR_WARN_OVERREV         */ "ATTENZIONE: FUORIGIRI!",
    /* STR_WARN_LOW_BAT         */ "ATTENZIONE: BATTERIA SCARICA!",

    /* STR_STATUS_SIMULATION    */ "MODALITA SIMULAZIONE",
    /* STR_STATUS_WIRELESS_OK   */ "APEX-TRACK CONNESSO",
    /* STR_STATUS_NO_SIGNAL     */ "NESSUN SEGNALE TRACK",
    /* STR_STATUS_USB_MSC_ACTIVE*/ "ARCHIVIAZIONE USB ATTIVA"
  },

  // =========================================================================
  // LANG_FR (Français)
  // =========================================================================
  {
    /* STR_MENU_TITLE           */ "MENU DE CONFIGURATION",
    /* STR_CAT_RACE_CONFIG      */ "1. Config Course & Kart",
    /* STR_CAT_RPM_ALARM       */ "2. Shift Lights & Alarmes",
    /* STR_CAT_TRACK_GPS        */ "3. Circuit & Base GPS",
    /* STR_CAT_STORAGE_PC       */ "4. Stockage & Sync PC",
    /* STR_CAT_DISPLAY_PWM      */ "5. Ecran & Retroeclairage",
    /* STR_CAT_SYSTEM_LANG      */ "6. Systeme & Langue",
    /* STR_CAT_SENSORS_INFO     */ "7. Capteurs & Diagnostic",

    /* STR_DRIVE_TYPE           */ "Type Transmission",
    /* STR_SHIFT_RPM            */ "Regime Changement",
    /* STR_MAX_RPM              */ "Echelle Regime Max",
    /* STR_OVERREV_RPM          */ "Alarme Surregime",
    /* STR_WATER_ALARM          */ "Alarme Temp Eau",
    /* STR_EGT_ALARM            */ "Alarme Temp EGT",
    /* STR_LED_BRIGHTNESS       */ "Luminosite LED RGB",
    /* STR_LED_TEST             */ "Test LED RGB",
    /* STR_RPM_DISP_MODE        */ "Affichage Barre RPM",
    /* STR_RPM_DISP_BOTH        */ "Les deux (Ecran + LED)",
    /* STR_RPM_DISP_DISPLAY     */ "Ecran Seul",
    /* STR_RPM_DISP_LEDS        */ "LEDs Seules",
    /* STR_BACKLIGHT_PWM        */ "Retroeclairage",
    /* STR_TRACK_SELECT         */ "Choisir Circuit",
    /* STR_TRACK_SD_LOAD        */ "Recharger Pistes SD",
    /* STR_USB_MSC_START        */ "Lancer Disque USB PC",
    /* STR_LANGUAGE             */ "Langue",
    /* STR_SHOW_SPEED           */ "Afficher Vitesse",
    /* STR_INVERT_DISP          */ "Polarite Ecran",
    /* STR_UNITS_SPEED          */ "Unite Vitesse",
    /* STR_UNITS_TEMP           */ "Unite Temperature",
    /* STR_RESET_CONFIG         */ "Reinit. Usine",

    /* STR_DRIVE_DIRECT         */ "Prise Directe (1-Vit)",
    /* STR_DRIVE_CLUTCH         */ "Embrayage (Rotax/X30)",
    /* STR_DRIVE_SHIFTER        */ "Boite KZ (6 Vitesses)",

    /* STR_LABEL_SPEED          */ "VIT",
    /* STR_LABEL_RPM            */ "TR/M",
    /* STR_LABEL_GEAR           */ "RAPP",
    /* STR_LABEL_LAP            */ "TOUR",
    /* STR_LABEL_BEST           */ "MEIL",
    /* STR_LABEL_LAST           */ "DERN",
    /* STR_LABEL_DELTA          */ "DELTA",
    /* STR_LABEL_PRED           */ "PRED",
    /* STR_LABEL_SECTOR         */ "SECT",
    /* STR_LABEL_WATER          */ "EAU",
    /* STR_LABEL_EGT            */ "EGT",
    /* STR_LABEL_AIR            */ "AIR",
    /* STR_LABEL_RH             */ "HR",
    /* STR_LABEL_SATS           */ "SAT",
    /* STR_LABEL_BAT            */ "BAT",
    /* STR_LABEL_HRS            */ "H MOT",

    /* STR_WARN_SHIFT           */ "** PASSER **",
    /* STR_WARN_OVERHEAT        */ "ATTENTION: SURCHAUFFE EAU!",
    /* STR_WARN_EGT             */ "ATTENTION: EGT ELEVE!",
    /* STR_WARN_OVERREV         */ "ATTENTION: SURREGIME!",
    /* STR_WARN_LOW_BAT         */ "ATTENTION: BATTERIE FAIBLE!",

    /* STR_STATUS_SIMULATION    */ "MODE SIMULATION",
    /* STR_STATUS_WIRELESS_OK   */ "APEX-TRACK CONNECTE",
    /* STR_STATUS_NO_SIGNAL     */ "SIGNAL TRACK PERDU",
    /* STR_STATUS_USB_MSC_ACTIVE*/ "STOCKAGE USB ACTIF"
  },

  // =========================================================================
  // LANG_DE (Deutsch)
  // =========================================================================
  {
    /* STR_MENU_TITLE           */ "EINSTELLUNGEN",
    /* STR_CAT_RACE_CONFIG      */ "1. Rennen & Kart Setup",
    /* STR_CAT_RPM_ALARM       */ "2. Schaltblitze & Alarme",
    /* STR_CAT_TRACK_GPS        */ "3. Strecke & GPS-Daten",
    /* STR_CAT_STORAGE_PC       */ "4. Speicher & PC-Sync",
    /* STR_CAT_DISPLAY_PWM      */ "5. Display & Beleuchtung",
    /* STR_CAT_SYSTEM_LANG      */ "6. System & Sprache",
    /* STR_CAT_SENSORS_INFO     */ "7. Sensoren & Diagnose",

    /* STR_DRIVE_TYPE           */ "Antriebsart",
    /* STR_SHIFT_RPM            */ "Schaltdrehzahl",
    /* STR_MAX_RPM              */ "Max Drehzahl",
    /* STR_OVERREV_RPM          */ "Uberdrehzahl-Alarm",
    /* STR_WATER_ALARM          */ "Wassertemp-Alarm",
    /* STR_EGT_ALARM            */ "Abgastemp-Alarm",
    /* STR_LED_BRIGHTNESS       */ "LED-Helligkeit",
    /* STR_LED_TEST             */ "RGB-LED Test",
    /* STR_RPM_DISP_MODE        */ "Drehzahlbalken-Modus",
    /* STR_RPM_DISP_BOTH        */ "Beide (Display + LEDs)",
    /* STR_RPM_DISP_DISPLAY     */ "Nur Display",
    /* STR_RPM_DISP_LEDS        */ "Nur LED-Leiste",
    /* STR_BACKLIGHT_PWM        */ "Displaybeleuchtung",

    /* STR_TRACK_SELECT         */ "Strecke Wahlen",
    /* STR_TRACK_SD_LOAD        */ "Strecken von SD Laden",
    /* STR_USB_MSC_START        */ "PC USB-Speicher Starten",
    /* STR_LANGUAGE             */ "Sprache",
    /* STR_SHOW_SPEED           */ "Geschw. Anzeigen",
    /* STR_INVERT_DISP          */ "Farbinversion",

    /* STR_UNITS_SPEED          */ "Geschw.-Einheit",
    /* STR_UNITS_TEMP           */ "Temp.-Einheit",
    /* STR_RESET_CONFIG         */ "Werkseinstellungen",

    /* STR_DRIVE_DIRECT         */ "Direktantrieb (1-Gang)",
    /* STR_DRIVE_CLUTCH         */ "Kupplung (OKJ/Rotax/X30)",
    /* STR_DRIVE_SHIFTER        */ "Schaltkart (KZ 6-Gang)",

    /* STR_LABEL_SPEED          */ "GESCH",
    /* STR_LABEL_RPM            */ "U/MIN",
    /* STR_LABEL_GEAR           */ "GANG",
    /* STR_LABEL_LAP            */ "RUNDE",
    /* STR_LABEL_BEST           */ "BEST",
    /* STR_LABEL_LAST           */ "LETZ",
    /* STR_LABEL_DELTA          */ "DELTA",
    /* STR_LABEL_PRED           */ "VORH",
    /* STR_LABEL_SECTOR         */ "SEKT",
    /* STR_LABEL_WATER          */ "WASSER",
    /* STR_LABEL_EGT            */ "EGT",
    /* STR_LABEL_AIR            */ "LUFT",
    /* STR_LABEL_RH             */ "RL",
    /* STR_LABEL_SATS           */ "SAT",
    /* STR_LABEL_BAT            */ "AKKU",
    /* STR_LABEL_HRS            */ "MOT STD",

    /* STR_WARN_SHIFT           */ "** SCHALTEN **",
    /* STR_WARN_OVERHEAT        */ "ACHTUNG: MOTOR UBERHITZT!",
    /* STR_WARN_EGT             */ "ACHTUNG: HOHE ABGASTEMP!",
    /* STR_WARN_OVERREV         */ "ACHTUNG: UBERDREHZAHL!",
    /* STR_WARN_LOW_BAT         */ "ACHTUNG: AKKU FAST LEER!",

    /* STR_STATUS_SIMULATION    */ "SIMULATIONSMODUS",
    /* STR_STATUS_WIRELESS_OK   */ "APEX-TRACK VERBUNDEN",
    /* STR_STATUS_NO_SIGNAL     */ "KEIN TRACK-SIGNAL",
    /* STR_STATUS_USB_MSC_ACTIVE*/ "USB-MASSENSPEICHER AKTIV"
  }
};

void I18n::setLanguage(Language lang) {
  if (lang < LANG_COUNT) {
    _currentLang = lang;
  }
}

Language I18n::getLanguage() {
  return _currentLang;
}

const char* I18n::getLanguageName(Language lang) {
  switch (lang) {
    case LANG_EN: return "English";
    case LANG_IT: return "Italiano";
    case LANG_FR: return "Francais";
    case LANG_DE: return "Deutsch";
    default: return "English";
  }
}

const char* I18n::get(StrId id) {
  if (id < STR_MAX_STRINGS) {
    return STRINGS[_currentLang][id];
  }
  return "";
}
