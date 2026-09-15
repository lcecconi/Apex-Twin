"""
Internationalization (i18n) Engine for Apex-Dash Emulator
Supports English (ENG), Italian (ITA), French (FRA), and German (GER).
"""

from enum import Enum
from emu.core.telemetry_model import Language


class StrId(Enum):
    MENU_TITLE = "MENU_TITLE"
    CAT_RACE_CONFIG = "CAT_RACE_CONFIG"
    CAT_RPM_ALARM = "CAT_RPM_ALARM"
    CAT_TRACK_GPS = "CAT_TRACK_GPS"
    CAT_STORAGE_PC = "CAT_STORAGE_PC"
    CAT_DISPLAY_PWM = "CAT_DISPLAY_PWM"
    CAT_SYSTEM_LANG = "CAT_SYSTEM_LANG"
    CAT_SENSORS_INFO = "CAT_SENSORS_INFO"

    DRIVE_TYPE = "DRIVE_TYPE"
    SHIFT_RPM = "SHIFT_RPM"
    MAX_RPM = "MAX_RPM"
    OVERREV_RPM = "OVERREV_RPM"
    WATER_ALARM = "WATER_ALARM"
    EGT_ALARM = "EGT_ALARM"
    LED_BRIGHTNESS = "LED_BRIGHTNESS"
    LED_TEST = "LED_TEST"
    RPM_DISP_MODE = "RPM_DISP_MODE"
    RPM_DISP_BOTH = "RPM_DISP_BOTH"
    RPM_DISP_DISPLAY = "RPM_DISP_DISPLAY"
    RPM_DISP_LEDS = "RPM_DISP_LEDS"
    BACKLIGHT_PWM = "BACKLIGHT_PWM"
    TRACK_SELECT = "TRACK_SELECT"
    TRACK_SD_LOAD = "TRACK_SD_LOAD"
    USB_MSC_START = "USB_MSC_START"
    LANGUAGE = "LANGUAGE"
    INVERT_DISP = "INVERT_DISP"
    UNITS_SPEED = "UNITS_SPEED"
    UNITS_TEMP = "UNITS_TEMP"
    RESET_CONFIG = "RESET_CONFIG"

    DRIVE_DIRECT = "DRIVE_DIRECT"
    DRIVE_CLUTCH = "DRIVE_CLUTCH"
    DRIVE_SHIFTER = "DRIVE_SHIFTER"

    LABEL_SPEED = "LABEL_SPEED"
    LABEL_RPM = "LABEL_RPM"
    LABEL_GEAR = "LABEL_GEAR"
    LABEL_LAP = "LABEL_LAP"
    LABEL_BEST = "LABEL_BEST"
    LABEL_LAST = "LABEL_LAST"
    LABEL_DELTA = "LABEL_DELTA"
    LABEL_PRED = "LABEL_PRED"
    LABEL_SECTOR = "LABEL_SECTOR"
    LABEL_WATER = "LABEL_WATER"
    LABEL_EGT = "LABEL_EGT"
    LABEL_AIR = "LABEL_AIR"
    LABEL_RH = "LABEL_RH"
    LABEL_SATS = "LABEL_SATS"
    LABEL_BAT = "LABEL_BAT"
    LABEL_HRS = "LABEL_HRS"

    WARN_SHIFT = "WARN_SHIFT"
    WARN_OVERHEAT = "WARN_OVERHEAT"
    WARN_EGT = "WARN_EGT"
    WARN_OVERREV = "WARN_OVERREV"
    WARN_LOW_BAT = "WARN_LOW_BAT"

    STATUS_SIMULATION = "STATUS_SIMULATION"
    STATUS_WIRELESS_OK = "STATUS_WIRELESS_OK"
    STATUS_NO_SIGNAL = "STATUS_NO_SIGNAL"
    STATUS_USB_MSC_ACTIVE = "STATUS_USB_MSC_ACTIVE"


STRINGS = {
    Language.LANG_EN: {
        StrId.MENU_TITLE: "SETUP MENU",
        StrId.CAT_RACE_CONFIG: "1. Race & Kart Setup",
        StrId.CAT_RPM_ALARM: "2. Shift Lights & Alarms",
        StrId.CAT_TRACK_GPS: "3. Track & GPS Database",
        StrId.CAT_STORAGE_PC: "4. Storage & PC Sync",
        StrId.CAT_DISPLAY_PWM: "5. Display & Backlight",
        StrId.CAT_SYSTEM_LANG: "6. System & Language",
        StrId.CAT_SENSORS_INFO: "7. Sensors & Diagnostics",
        StrId.DRIVE_TYPE: "Drive Type",
        StrId.SHIFT_RPM: "Shift RPM",
        StrId.MAX_RPM: "Max RPM Scale",
        StrId.OVERREV_RPM: "Over-Rev Alarm",
        StrId.WATER_ALARM: "Water Temp Alarm",
        StrId.EGT_ALARM: "EGT Temp Alarm",
        StrId.LED_BRIGHTNESS: "RGB LED Brightness",
        StrId.LED_TEST: "Test RGB LEDs",
        StrId.RPM_DISP_MODE: "RPM Bar Display",
        StrId.RPM_DISP_BOTH: "Both (LCD + LEDs)",
        StrId.RPM_DISP_DISPLAY: "Display Only",
        StrId.RPM_DISP_LEDS: "LED Strip Only",
        StrId.BACKLIGHT_PWM: "Backlight (PWM)",
        StrId.TRACK_SELECT: "Select Track",
        StrId.TRACK_SD_LOAD: "Reload Tracks from SD",
        StrId.USB_MSC_START: "Start PC USB Drive",
        StrId.LANGUAGE: "Language",
        StrId.INVERT_DISP: "Display Polarity",
        StrId.UNITS_SPEED: "Speed Unit",
        StrId.UNITS_TEMP: "Temp Unit",
        StrId.RESET_CONFIG: "Factory Reset",
        StrId.DRIVE_DIRECT: "Direct Drive (1-Speed)",
        StrId.DRIVE_CLUTCH: "Clutch (OKJ/Rotax/X30)",
        StrId.DRIVE_SHIFTER: "Shifter (KZ 6-Speed)",
        StrId.LABEL_SPEED: "SPEED",
        StrId.LABEL_RPM: "RPM",
        StrId.LABEL_GEAR: "GEAR",
        StrId.LABEL_LAP: "LAP",
        StrId.LABEL_BEST: "BEST",
        StrId.LABEL_LAST: "LAST",
        StrId.LABEL_DELTA: "DELTA",
        StrId.LABEL_PRED: "PRED",
        StrId.LABEL_SECTOR: "SEC",
        StrId.LABEL_WATER: "WATER",
        StrId.LABEL_EGT: "EGT",
        StrId.LABEL_AIR: "AIR",
        StrId.LABEL_RH: "RH",
        StrId.LABEL_SATS: "SATS",
        StrId.LABEL_BAT: "BAT",
        StrId.LABEL_HRS: "ENG HRS",
        StrId.WARN_SHIFT: "** SHIFT **",
        StrId.WARN_OVERHEAT: "WARN: WATER OVERHEAT!",
        StrId.WARN_EGT: "WARN: HIGH EGT!",
        StrId.WARN_OVERREV: "WARN: OVER-REV!",
        StrId.WARN_LOW_BAT: "WARN: LOW BATTERY!",
        StrId.STATUS_SIMULATION: "SIMULATION MODE",
        StrId.STATUS_WIRELESS_OK: "APEX-TRACK LINKED",
        StrId.STATUS_NO_SIGNAL: "NO APEX-TRACK LINK",
        StrId.STATUS_USB_MSC_ACTIVE: "USB MASS STORAGE ACTIVE",
    },
    Language.LANG_IT: {
        StrId.MENU_TITLE: "MENU IMPOSTAZIONI",
        StrId.CAT_RACE_CONFIG: "1. Setup Gara & Kart",
        StrId.CAT_RPM_ALARM: "2. Giri & Allarmi LED",
        StrId.CAT_TRACK_GPS: "3. Pista & Database GPS",
        StrId.CAT_STORAGE_PC: "4. Memoria & Sync PC",
        StrId.CAT_DISPLAY_PWM: "5. Display & Retroillum.",
        StrId.CAT_SYSTEM_LANG: "6. Sistema & Lingua",
        StrId.CAT_SENSORS_INFO: "7. Sensori & Diagnostica",
        StrId.DRIVE_TYPE: "Tipo Trasmissione",
        StrId.SHIFT_RPM: "Giri Cambiata",
        StrId.MAX_RPM: "Giri Fondo Scala",
        StrId.OVERREV_RPM: "Allarme Fuorigiri",
        StrId.WATER_ALARM: "Allarme Temp Acqua",
        StrId.EGT_ALARM: "Allarme Temp EGT",
        StrId.LED_BRIGHTNESS: "Luminosita LED RGB",
        StrId.LED_TEST: "Test LED RGB",
        StrId.RPM_DISP_MODE: "Mostra Barra Giri",
        StrId.RPM_DISP_BOTH: "Entrambi (LCD + LED)",
        StrId.RPM_DISP_DISPLAY: "Solo Display",
        StrId.RPM_DISP_LEDS: "Solo Striscia LED",
        StrId.BACKLIGHT_PWM: "Retroilluminazione",
        StrId.TRACK_SELECT: "Seleziona Circuito",
        StrId.TRACK_SD_LOAD: "Ricarica Piste da SD",
        StrId.USB_MSC_START: "Avvia Storage USB PC",
        StrId.LANGUAGE: "Lingua",
        StrId.INVERT_DISP: "Polarita Display",
        StrId.UNITS_SPEED: "Unita Velocita",
        StrId.UNITS_TEMP: "Unita Temperatura",
        StrId.RESET_CONFIG: "Ripristino Fabbrica",
        StrId.DRIVE_DIRECT: "Presa Diretta (1-Marcia)",
        StrId.DRIVE_CLUTCH: "Frizione (OKJ/Rotax/X30)",
        StrId.DRIVE_SHIFTER: "KZ Cambio (6 Marce)",
        StrId.LABEL_SPEED: "VEL",
        StrId.LABEL_RPM: "GIRI",
        StrId.LABEL_GEAR: "MARC",
        StrId.LABEL_LAP: "GIRO",
        StrId.LABEL_BEST: "BEST",
        StrId.LABEL_LAST: "ULT",
        StrId.LABEL_DELTA: "DELTA",
        StrId.LABEL_PRED: "PRED",
        StrId.LABEL_SECTOR: "SETT",
        StrId.LABEL_WATER: "ACQUA",
        StrId.LABEL_EGT: "EGT",
        StrId.LABEL_AIR: "ARIA",
        StrId.LABEL_RH: "UR",
        StrId.LABEL_SATS: "SAT",
        StrId.LABEL_BAT: "BAT",
        StrId.LABEL_HRS: "ORE MOT",
        StrId.WARN_SHIFT: "** CAMBIO **",
        StrId.WARN_OVERHEAT: "ATTENZIONE: SURRISC. ACQUA!",
        StrId.WARN_EGT: "ATTENZIONE: EGT ELEVATO!",
        StrId.WARN_OVERREV: "ATTENZIONE: FUORIGIRI!",
        StrId.WARN_LOW_BAT: "ATTENZIONE: BATTERIA SCARICA!",
        StrId.STATUS_SIMULATION: "MODALITA SIMULAZIONE",
        StrId.STATUS_WIRELESS_OK: "APEX-TRACK CONNESSO",
        StrId.STATUS_NO_SIGNAL: "NESSUN SEGNALE TRACK",
        StrId.STATUS_USB_MSC_ACTIVE: "ARCHIVIAZIONE USB ATTIVA",
    },
    Language.LANG_FR: {
        StrId.MENU_TITLE: "MENU DE CONFIGURATION",
        StrId.CAT_RACE_CONFIG: "1. Config Course & Kart",
        StrId.CAT_RPM_ALARM: "2. Shift Lights & Alarmes",
        StrId.CAT_TRACK_GPS: "3. Circuit & Base GPS",
        StrId.CAT_STORAGE_PC: "4. Stockage & Sync PC",
        StrId.CAT_DISPLAY_PWM: "5. Ecran & Retroeclairage",
        StrId.CAT_SYSTEM_LANG: "6. Systeme & Langue",
        StrId.CAT_SENSORS_INFO: "7. Capteurs & Diagnostic",
        StrId.DRIVE_TYPE: "Type Transmission",
        StrId.SHIFT_RPM: "Regime Changement",
        StrId.MAX_RPM: "Echelle Regime Max",
        StrId.OVERREV_RPM: "Alarme Surregime",
        StrId.WATER_ALARM: "Alarme Temp Eau",
        StrId.EGT_ALARM: "Alarme Temp EGT",
        StrId.LED_BRIGHTNESS: "Luminosite LED RGB",
        StrId.LED_TEST: "Test LED RGB",
        StrId.RPM_DISP_MODE: "Affichage Barre RPM",
        StrId.RPM_DISP_BOTH: "Les deux (Ecran + LED)",
        StrId.RPM_DISP_DISPLAY: "Ecran Seul",
        StrId.RPM_DISP_LEDS: "LEDs Seules",
        StrId.BACKLIGHT_PWM: "Retroeclairage",
        StrId.TRACK_SELECT: "Choisir Circuit",
        StrId.TRACK_SD_LOAD: "Recharger Pistes SD",
        StrId.USB_MSC_START: "Lancer Disque USB PC",
        StrId.LANGUAGE: "Langue",
        StrId.INVERT_DISP: "Polarite Ecran",
        StrId.UNITS_SPEED: "Unite Vitesse",
        StrId.UNITS_TEMP: "Unite Temperature",
        StrId.RESET_CONFIG: "Reinit. Usine",
        StrId.DRIVE_DIRECT: "Prise Directe (1-Vit)",
        StrId.DRIVE_CLUTCH: "Embrayage (Rotax/X30)",
        StrId.DRIVE_SHIFTER: "Boite KZ (6 Vitesses)",
        StrId.LABEL_SPEED: "VIT",
        StrId.LABEL_RPM: "TR/M",
        StrId.LABEL_GEAR: "RAPP",
        StrId.LABEL_LAP: "TOUR",
        StrId.LABEL_BEST: "MEIL",
        StrId.LABEL_LAST: "DERN",
        StrId.LABEL_DELTA: "DELTA",
        StrId.LABEL_PRED: "PRED",
        StrId.LABEL_SECTOR: "SECT",
        StrId.LABEL_WATER: "EAU",
        StrId.LABEL_EGT: "EGT",
        StrId.LABEL_AIR: "AIR",
        StrId.LABEL_RH: "HR",
        StrId.LABEL_SATS: "SAT",
        StrId.LABEL_BAT: "BAT",
        StrId.LABEL_HRS: "H MOT",
        StrId.WARN_SHIFT: "** PASSER **",
        StrId.WARN_OVERHEAT: "ATTENTION: SURCHAUFFE EAU!",
        StrId.WARN_EGT: "ATTENTION: EGT ELEVE!",
        StrId.WARN_OVERREV: "ATTENTION: SURREGIME!",
        StrId.WARN_LOW_BAT: "ATTENTION: BATTERIE FAIBLE!",
        StrId.STATUS_SIMULATION: "MODE SIMULATION",
        StrId.STATUS_WIRELESS_OK: "APEX-TRACK CONNECTE",
        StrId.STATUS_NO_SIGNAL: "SIGNAL TRACK PERDU",
        StrId.STATUS_USB_MSC_ACTIVE: "STOCKAGE USB ACTIF",
    },
    Language.LANG_DE: {
        StrId.MENU_TITLE: "EINSTELLUNGEN",
        StrId.CAT_RACE_CONFIG: "1. Rennen & Kart Setup",
        StrId.CAT_RPM_ALARM: "2. Schaltblitze & Alarme",
        StrId.CAT_TRACK_GPS: "3. Strecke & GPS-Daten",
        StrId.CAT_STORAGE_PC: "4. Speicher & PC-Sync",
        StrId.CAT_DISPLAY_PWM: "5. Display & Beleuchtung",
        StrId.CAT_SYSTEM_LANG: "6. System & Sprache",
        StrId.CAT_SENSORS_INFO: "7. Sensoren & Diagnose",
        StrId.DRIVE_TYPE: "Antriebsart",
        StrId.SHIFT_RPM: "Schaltdrehzahl",
        StrId.MAX_RPM: "Max Drehzahl",
        StrId.OVERREV_RPM: "Uberdrehzahl-Alarm",
        StrId.WATER_ALARM: "Wassertemp-Alarm",
        StrId.EGT_ALARM: "Abgastemp-Alarm",
        StrId.LED_BRIGHTNESS: "LED-Helligkeit",
        StrId.LED_TEST: "RGB-LED Test",
        StrId.RPM_DISP_MODE: "Drehzahlbalken-Modus",
        StrId.RPM_DISP_BOTH: "Beide (Display + LEDs)",
        StrId.RPM_DISP_DISPLAY: "Nur Display",
        StrId.RPM_DISP_LEDS: "Nur LED-Leiste",
        StrId.BACKLIGHT_PWM: "Displaybeleuchtung",

        StrId.TRACK_SELECT: "Strecke Wahlen",
        StrId.TRACK_SD_LOAD: "Strecken von SD Laden",
        StrId.USB_MSC_START: "PC USB-Speicher Starten",
        StrId.LANGUAGE: "Sprache",
        StrId.INVERT_DISP: "Farbinversion",
        StrId.UNITS_SPEED: "Geschw.-Einheit",
        StrId.UNITS_TEMP: "Temp.-Einheit",
        StrId.RESET_CONFIG: "Werkseinstellungen",
        StrId.DRIVE_DIRECT: "Direktantrieb (1-Gang)",
        StrId.DRIVE_CLUTCH: "Kupplung (OKJ/Rotax/X30)",
        StrId.DRIVE_SHIFTER: "Schaltkart (KZ 6-Gang)",
        StrId.LABEL_SPEED: "GESCH",
        StrId.LABEL_RPM: "U/MIN",
        StrId.LABEL_GEAR: "GANG",
        StrId.LABEL_LAP: "RUNDE",
        StrId.LABEL_BEST: "BEST",
        StrId.LABEL_LAST: "LETZ",
        StrId.LABEL_DELTA: "DELTA",
        StrId.LABEL_PRED: "VORH",
        StrId.LABEL_SECTOR: "SEKT",
        StrId.LABEL_WATER: "WASSER",
        StrId.LABEL_EGT: "EGT",
        StrId.LABEL_AIR: "LUFT",
        StrId.LABEL_RH: "RL",
        StrId.LABEL_SATS: "SAT",
        StrId.LABEL_BAT: "AKKU",
        StrId.LABEL_HRS: "MOT STD",
        StrId.WARN_SHIFT: "** SCHALTEN **",
        StrId.WARN_OVERHEAT: "ACHTUNG: MOTOR UBERHITZT!",
        StrId.WARN_EGT: "ACHTUNG: HOHE ABGASTEMP!",
        StrId.WARN_OVERREV: "ACHTUNG: UBERDREHZAHL!",
        StrId.WARN_LOW_BAT: "ACHTUNG: AKKU FAST LEER!",
        StrId.STATUS_SIMULATION: "SIMULATIONSMODUS",
        StrId.STATUS_WIRELESS_OK: "APEX-TRACK VERBUNDEN",
        StrId.STATUS_NO_SIGNAL: "KEIN TRACK-SIGNAL",
        StrId.STATUS_USB_MSC_ACTIVE: "USB-MASSENSPEICHER AKTIV",
    }
}


class I18n:
    _current_lang = Language.LANG_EN

    @classmethod
    def set_language(cls, lang: Language):
        cls._current_lang = lang

    @classmethod
    def get_language(cls) -> Language:
        return cls._current_lang

    @classmethod
    def get(cls, str_id: StrId) -> str:
        lang_dict = STRINGS.get(cls._current_lang, STRINGS[Language.LANG_EN])
        return lang_dict.get(str_id, str_id.value)

    @classmethod
    def get_language_name(cls, lang: Language) -> str:
        names = {
            Language.LANG_EN: "English",
            Language.LANG_IT: "Italiano",
            Language.LANG_FR: "Francais",
            Language.LANG_DE: "Deutsch",
        }
        return names.get(lang, "English")
