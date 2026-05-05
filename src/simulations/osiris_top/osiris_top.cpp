#include "osiris_top.h"
#include "src/glyphs/osiris_icons/osiris_icons.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "lvgl/lvgl.h"

#define DISPLAY_WIDTH  800
#define DISPLAY_HEIGHT 480


// ─── Enums ───────────────────────────────────────────────────────────────────
enum HaDomain : uint8_t {
    HaD_Light=0, HaD_Switch, HaD_Climate, HaD_Sensor, HaD_BinarySensor, HaD_Cover, HaD_Camera, HaD_Script
};

enum HaCategory : uint8_t {
    HaCat_LICHT = 0,
    HaCat_SCHALTER = 1,
    HaCat_KLIMA = 2,
    HaCat_SENSOR = 3,
    HaCat_FENSTER = 4,
    HaCat_ENERGIE = 5,
    HaCat_KAMERA = 6,
    HaCat_SKRIPTE = 7
};
#define HA_CATEGORY_COUNT 8

enum HaRoom : uint8_t {
    HaRoom_DACHBODEN = 0,
    HaRoom_BAD = 1,
    HaRoom_ANKLEIDEZIMMER = 2,
    HaRoom_BUERO = 3,
    HaRoom_SCHLAFZIMMER = 4,
    HaRoom_TREPPE = 5,
    HaRoom_ERSTER_STOCK = 6,
    HaRoom_ABSTELLRAUM = 7,
    HaRoom_FLUR = 8,
    HaRoom_KUECHE = 9,
    HaRoom_WOHNZIMMER = 10,
    HaRoom_PANTRY = 11,
    HaRoom_KLO = 12,
    HaRoom_EINGANG = 13,
    HaRoom_GARTEN = 14,
    HaRoom_EINFAHRT = 15,
    HaRoom_KELLER = 16,
    HaRoom_HEIZUNG = 17,
    HaRoom_LABOR = 18,
    HaRoom_CONTROLROOM = 19,
    HaRoom_WASCHKUECHE = 20
};
#define HA_ROOM_COUNT 21

enum HaService : uint8_t {
    HaSvc_Toggle=0,    // light (on/off only) or switch
    HaSvc_DimToggle,   // light with brightness support
    HaSvc_CoverToggle, // cover
    HaSvc_ScriptRun,   // script (trigger via script/turn_on)
    HaSvc_None         // read-only sensor/binary_sensor/climate/camera
};

struct HaEntity {
    const char* entity_id;
    const char* name;
    enum HaDomain    domain;
    enum HaService   service;
};

#define HA_ENTITY_COUNT 194
static const struct HaEntity HA_ENTITIES[] = {
    {"light.dach", "Roo Light", HaD_Light, HaSvc_Toggle},
    {"sensor.roo_temp_hum_sensor_temperature", "Roo Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.roo_temp_hum_sensor_humidity", "Roo Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"binary_sensor.roo_window_sensor_a_contact", "Roo Window Sensor A Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.roo_window_sensor_c_contact", "Roo Window Sensor C Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.roo_window_sensor_d_contact", "Roo Window Sensor D Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.roo_window_sensor_b_contact", "Roo Window Sensor B Tür", HaD_BinarySensor, HaSvc_None},
    {"climate.bat_radiator_thermostat", "Bat Radiator Thermostat", HaD_Climate, HaSvc_None},
    {"climate.bat_thermostat", "Bat Thermostat", HaD_Climate, HaSvc_None},
    {"light.bat_light", "Bat Light", HaD_Light, HaSvc_Toggle},
    {"binary_sensor.bat_window_sensor_a_contact", "Bat Window Sensor A Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.bat_window_sensor_b_contact", "Bat Window Sensor B Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.wise_bat_upf_contact", "WiSe Bat_Upf Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.bat_temp_hum_sensor_temperature", "Bat Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.bat_temp_hum_sensor_humidity", "Bat Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"climate.dre_thermostat", "Dre Thermostat", HaD_Climate, HaSvc_None},
    {"sensor.dre_temp_hum_sensor_temperature", "Dre Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.dre_temp_hum_sensor_humidity", "Dre Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"binary_sensor.wise_dre_upf_contact", "WiSe Dre_Upf Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.dre_pirmot_illuminance", "Dre PirMot Beleuchtungsstärke", HaD_Sensor, HaSvc_None},
    {"climate.off_thermostat", "Off Thermostat", HaD_Climate, HaSvc_None},
    {"cover.off_shutter_window", "Off Shutter Window", HaD_Cover, HaSvc_CoverToggle},
    {"light.off_light_l1", "Light", HaD_Light, HaSvc_Toggle},
    {"binary_sensor.off_window_sensor_a_contact", "Off Window Sensor A Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.wise_off_upf_contact", "WiSe Off_Upf Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.off_window_sensor_b_contact", "Off Window Sensor B Tür", HaD_BinarySensor, HaSvc_None},
    {"light.sonoff_100135c1bc_1", "Sle FanLight", HaD_Light, HaSvc_Toggle},
    {"light.smart_star_projector_background", "Sle Smart Star Projector Background", HaD_Light, HaSvc_DimToggle},
    {"light.smart_star_projector_laser", "Sle Smart Star Projector Laser", HaD_Light, HaSvc_DimToggle},
    {"climate.sle_thermostat", "Sle Thermostat", HaD_Climate, HaSvc_None},
    {"cover.sle_shutter_window", "Sle Shutter Window", HaD_Cover, HaSvc_CoverToggle},
    {"binary_sensor.sle_window_sensor_contact", "Sle Window Sensor Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.sle_temp_hum_sensor_temperature", "Sle Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.sle_temp_hum_sensor_humidity", "Sle Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"switch.sle_tv", "Sle TV", HaD_Switch, HaSvc_Toggle},
    {"binary_sensor.wise_sle_upf_contact", "WiSe Sle_Upf Tür", HaD_BinarySensor, HaSvc_None},
    {"light.upf_light_ceiling", "Upf Light Ceiling", HaD_Light, HaSvc_Toggle},
    {"sensor.cle_pirmot_illuminance", "Cle PirMot Beleuchtungsstärke", HaD_Sensor, HaSvc_None},
    {"binary_sensor.wise_cle_flo_contact", "WiSe Cle_Flo Tür", HaD_BinarySensor, HaSvc_None},
    {"camera.g4_instant_high_resolution_channel", "G4 Instant Flur High resolution channel", HaD_Camera, HaSvc_None},
    {"climate.flo_thermostat", "Flo Thermostat", HaD_Climate, HaSvc_None},
    {"light.flo_light_ceiling", "Flo Light Ceiling", HaD_Light, HaSvc_Toggle},
    {"switch.flo_siren_alarm", "Flo Siren Alarm", HaD_Switch, HaSvc_Toggle},
    {"light.flo_ledstrip", "Flo LEDStrip", HaD_Light, HaSvc_DimToggle},
    {"binary_sensor.flo_window_sensor_contact", "Flo Window Sensor Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.flo_temp_hum_sensor_temperature", "Flo Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.flo_temp_hum_sensor_humidity", "Flo Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"climate.kit_thermostat", "Kit Thermostat", HaD_Climate, HaSvc_None},
    {"cover.kit_shutter_window", "Kit Shutter Window", HaD_Cover, HaSvc_CoverToggle},
    {"binary_sensor.kit_window_sensor_contact", "Kit Window Sensor Tür", HaD_BinarySensor, HaSvc_None},
    {"light.kit_light_1", "Kit Light 1", HaD_Light, HaSvc_Toggle},
    {"light.kit_light_buffet", "Kit Light Buffet", HaD_Light, HaSvc_Toggle},
    {"light.kit_light_line", "Kit Light Line", HaD_Light, HaSvc_Toggle},
    {"binary_sensor.wise_kit_flo_contact", "WiSe Kit_Flo Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.kit_temp_hum_sensor_temperature", "Kit Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.kit_temp_hum_sensor_humidity", "Kit Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"light.sonoff_10018427b3_1", "Liv EatingTable", HaD_Light, HaSvc_Toggle},
    {"light.wohnzimmerlufter", "Couch", HaD_Light, HaSvc_DimToggle},
    {"light.wohnzimmertv_backlight", "Liv TV Backlight", HaD_Light, HaSvc_DimToggle},
    {"climate.liv_thermostat", "Liv Thermostat", HaD_Climate, HaSvc_None},
    {"cover.liv_shutter_patio_door", "Liv Shutter Patio Door", HaD_Cover, HaSvc_CoverToggle},
    {"cover.liv_shutter_window_runway", "Liv Shutter Window Runway", HaD_Cover, HaSvc_CoverToggle},
    {"cover.liv_shutter_window_garden", "Liv Shutter Window Garden", HaD_Cover, HaSvc_CoverToggle},
    {"light.liv_light_4", "Liv Light 4", HaD_Light, HaSvc_DimToggle},
    {"light.liv_light_1", "Liv Light 1", HaD_Light, HaSvc_Toggle},
    {"light.liv_socket_1", "HeartOfGlass", HaD_Light, HaSvc_Toggle},
    {"light.liv_socket_2", "HexagonWall", HaD_Light, HaSvc_Toggle},
    {"switch.liv_tv", "Liv TV", HaD_Switch, HaSvc_Toggle},
    {"switch.liv_mosquitokiller", "Liv MosquitoKiller", HaD_Switch, HaSvc_Toggle},
    {"binary_sensor.wise_liv_flo_contact", "WiSe Liv_Flo Tür", HaD_BinarySensor, HaSvc_None},
    {"switch.liv_sidematrix", "Liv SideMatrix", HaD_Switch, HaSvc_Toggle},
    {"switch.liv_backspeaker", "Liv Backspeaker", HaD_Switch, HaSvc_Toggle},
    {"sensor.liv_temp_hum_sensor_temperature", "Liv Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.liv_temp_hum_sensor_humidity", "Liv Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"binary_sensor.liv_window_sensor_a_contact", "Liv Window Sensor A Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.liv_window_sensor_c_contact", "Liv Window Sensor C Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.liv_window_sensor_b_contact", "Liv Window Sensor B Tür", HaD_BinarySensor, HaSvc_None},
    {"switch.liv_usb_l1", "L1 Ecowitt", HaD_Switch, HaSvc_Toggle},
    {"switch.liv_usb_l2", "L2 Broadlink", HaD_Switch, HaSvc_Toggle},
    {"sensor.pan_pirmot_illuminance", "Pan PirMot Beleuchtungsstärke", HaD_Sensor, HaSvc_None},
    {"binary_sensor.wise_pan_kit_contact", "WiSe Pan_Kit Tür", HaD_BinarySensor, HaSvc_None},
    {"climate.toi_thermostat", "Toi Thermostat", HaD_Climate, HaSvc_None},
    {"light.toi_light", "Toi Light", HaD_Light, HaSvc_Toggle},
    {"binary_sensor.wise_toi_flo_contact", "WiSe Toi_Flo Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.toi_window_sensor_contact", "Toi Window Sensor Tür", HaD_BinarySensor, HaSvc_None},
    {"camera.g4_doorbell_pro_poe_high_resolution_channel", "G4 Doorbell Pro PoE High resolution channel", HaD_Camera, HaSvc_None},
    {"camera.g4_doorbell_pro_poe_package_camera", "G4 Doorbell Pro PoE Package Camera", HaD_Camera, HaSvc_None},
    {"camera.g5_turret_ultra_high_resolution_channel_2", "G5 Turret Ultra Garten High resolution channel", HaD_Camera, HaSvc_None},
    {"sensor.solarflow_800_pro_pack_input_power", "SolarFlow 800 Pro Batterieentladeleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_output_pack_power", "SolarFlow 800 Pro Batterieladeleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_grid_input_power", "SolarFlow 800 Pro Netz-Eingangsleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_solar_power1", "SolarFlow 800 Pro PV1 Solarleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_solar_power2", "SolarFlow 800 Pro PV2 Solarleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_solar_power3", "SolarFlow 800 Pro PV3 Solarleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_solar_power4", "SolarFlow 800 Pro PV4 Solarleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_inverse_max_power", "SolarFlow 800 Pro behördl. begrenzte Ausgangsleistung", HaD_Sensor, HaSvc_None},
    {"sensor.solarflow_800_pro_aggr_solar", "SolarFlow 800 Pro Gesamte Solarenergie", HaD_Sensor, HaSvc_None},
    {"sensor.out_sensor_garden_temperature", "Gar MotionLight Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.out_sensor_garden_illuminance", "Gar MotionLight Beleuchtungsstärke", HaD_Sensor, HaSvc_None},
    {"camera.g5_turret_ultra_high_resolution_channel", "G5 Turret Ultra Einfahrt High resolution channel", HaD_Camera, HaSvc_None},
    {"camera.g5_ptz_high_resolution_channel", "G5 PTZ Einfahrt High resolution channel", HaD_Camera, HaSvc_None},
    {"light.wled_2", "Run LEDStripe", HaD_Light, HaSvc_DimToggle},
    {"light.run_wled_short", "light.run_wled_short", HaD_Light, HaSvc_DimToggle},
    {"light.run_carport_wled", "Run Carport Entrance", HaD_Light, HaSvc_DimToggle},
    {"light.run_carport_wled_segment_1", "Run Carport Runway", HaD_Light, HaSvc_DimToggle},
    {"light.bas_light_1", "Bas Light 1", HaD_Light, HaSvc_Toggle},
    {"switch.bas_heating", "Bas Heating", HaD_Switch, HaSvc_Toggle},
    {"light.bas_light_2", "Bas Light 2", HaD_Light, HaSvc_Toggle},
    {"binary_sensor.wise_hea_bas_contact", "WiSe Hea_Bas Tür", HaD_BinarySensor, HaSvc_None},
    {"light.lab_fan", "light.lab_fan", HaD_Light, HaSvc_DimToggle},
    {"climate.lab_thermostat", "Lab Thermostat", HaD_Climate, HaSvc_None},
    {"switch.lab_socket_desktop", "Lab Socket Desktop", HaD_Switch, HaSvc_Toggle},
    {"switch.lab_socket_printer", "Lab Socket Printer", HaD_Switch, HaSvc_Toggle},
    {"binary_sensor.lab_window_sensor_a_contact", "Lab Window Sensor A Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.wise_lab_bas_contact", "WiSe Lab_Bas Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.lab_temp_hum_sensor_temperature", "Lab Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.lab_temp_hum_sensor_humidity", "Lab Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
    {"binary_sensor.lab_window_sensor_b_contact", "Lab Window Sensor B Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.lab_window_sensor_c_contact", "Lab Window Sensor C Tür", HaD_BinarySensor, HaSvc_None},
    {"script.ambiente_bright_down", "Ambiente Bright Down", HaD_Script, HaSvc_ScriptRun},
    {"script.ambiente_bright_up", "Ambiente Bright Up", HaD_Script, HaSvc_ScriptRun},
    {"script.ambiente_lavendel", "Ambiente Lavendel ", HaD_Script, HaSvc_ScriptRun},
    {"script.bedtime", "Bedtime ", HaD_Script, HaSvc_ScriptRun},
    {"script.eg_cover", "EG Cover", HaD_Script, HaSvc_ScriptRun},
    {"script.fireplace_fire", "Fireplace Fire", HaD_Script, HaSvc_ScriptRun},
    {"script.fireplace_heat", "Fireplace Heat", HaD_Script, HaSvc_ScriptRun},
    {"script.fireplace_power", "Fireplace Power", HaD_Script, HaSvc_ScriptRun},
    {"script.lrtoggleambiente", "LRToggleAmbiente", HaD_Script, HaSvc_ScriptRun},
    {"script.lrtoggletv", "LRToggleTV", HaD_Script, HaSvc_ScriptRun},
    {"script.runway_light_off", "Runway Light Off", HaD_Script, HaSvc_ScriptRun},
    {"script.runway_light_on", "Runway Light On", HaD_Script, HaSvc_ScriptRun},
    {"sensor.tasmota_sgm_c8_total", "Tasmota SGM-C8 Total", HaD_Sensor, HaSvc_None},
    {"sensor.tasmota_sgm_c8_power", "Tasmota SGM-C8 Power", HaD_Sensor, HaSvc_None},
    {"sensor.gw3000a_humidity", "GW3000A Humidity", HaD_Sensor, HaSvc_None},
    {"sensor.gw3000a_solar_lux", "GW3000A Solar Lux", HaD_Sensor, HaSvc_None},
    {"sensor.gw3000a_outdoor_temperature", "GW3000A Outdoor Temperature", HaD_Sensor, HaSvc_None},
    {"sensor.gw3000a_absolute_pressure", "GW3000A Absolute Pressure", HaD_Sensor, HaSvc_None},
    {"sensor.gw3000a_dewpoint", "GW3000A Dewpoint", HaD_Sensor, HaSvc_None},
    {"switch.hsr01", "Hsr01", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr01_power", "Hsr01 Leistung", HaD_Sensor, HaSvc_None},
    {"sensor.dinrail_rccb_power_a", "Dinrail RCCB Leistung", HaD_Sensor, HaSvc_None},
    {"sensor.dinrail_rccb_power_b", "Dinrail RCCB Leistung", HaD_Sensor, HaSvc_None},
    {"sensor.dinrail_rccb_power_c", "Dinrail RCCB Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr03", "Hsr03", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr03_power", "Hsr03 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr05", "Hsr05", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr05_power", "Hsr05 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr06", "Hsr06", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr06_power", "Hsr06 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr04", "Hsr04", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr04_power", "Hsr04 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr02", "Hsr02", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr02_power", "Hsr02 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr10", "Hsr10", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr10_power", "Hsr10 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr11", "Hsr11", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr11_power", "Hsr11 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr14", "Hsr14", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr14_power", "Hsr14 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr13", "Hsr13", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr13_power", "Hsr13 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr12", "Hsr12", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr12_power", "Hsr12 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr17", "Hsr17", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr17_power", "Hsr17 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr19", "Hsr19", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr19_power", "Hsr19 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr15", "Hsr15", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr15_power", "Hsr15 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr18", "Hsr18", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr18_power", "Hsr18 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr09", "Hsr09", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr09_power", "Hsr09 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr08", "Waschmaschine", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr08_power", "Hsr08 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr16", "Hsr16", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr16_power", "Hsr16 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr07", "Trockner", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr07_power", "Hsr07 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr20", "Hsr20", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr20_power", "Hsr20 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr21", "Hsr21", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr21_power", "Hsr21 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr22", "Hsr22", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr22_power", "Hsr22 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr24", "Hsr24", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr24_power", "Hsr24 Leistung", HaD_Sensor, HaSvc_None},
    {"switch.hsr23", "Hsr23", HaD_Switch, HaSvc_Toggle},
    {"sensor.hsr23_power", "Hsr23 Leistung", HaD_Sensor, HaSvc_None},
    {"climate.lau_thermostat", "Lau Thermostat", HaD_Climate, HaSvc_None},
    {"binary_sensor.wise_lau_bas_contact", "WiSe Lau_Bas Tür", HaD_BinarySensor, HaSvc_None},
    {"binary_sensor.lau_window_sensor_contact", "Lau Window Sensor Tür", HaD_BinarySensor, HaSvc_None},
    {"sensor.lau_temp_hum_sensor_temperature", "Lau Temp Hum Sensor Temperatur", HaD_Sensor, HaSvc_None},
    {"sensor.lau_temp_hum_sensor_humidity", "Lau Temp Hum Sensor Luftfeuchtigkeit", HaD_Sensor, HaSvc_None},
};

// ─── Category indices ────────────────────────────────────────────────────────
static const uint16_t HA_CAT_LICHT_IDX[] = {0, 9, 22, 26, 27, 28, 36, 41, 43, 50, 51, 52, 56, 57, 58, 63, 64, 65, 66, 82, 101, 102, 103, 104, 105, 107, 109};
static const uint16_t HA_CAT_SCHALTER_IDX[] = {34, 42, 67, 68, 70, 71, 77, 78, 106, 111, 112, 138, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187};
static const uint16_t HA_CAT_KLIMA_IDX[] = {7, 8, 15, 20, 29, 40, 47, 59, 81, 110, 189};
static const uint16_t HA_CAT_SENSOR_IDX[] = {1, 2, 13, 14, 16, 17, 19, 32, 33, 37, 45, 46, 54, 55, 72, 73, 79, 97, 98, 115, 116, 133, 134, 135, 136, 137, 192, 193};
static const uint16_t HA_CAT_FENSTER_IDX[] = {3, 4, 5, 6, 10, 11, 12, 18, 21, 23, 24, 25, 30, 31, 35, 38, 44, 48, 49, 53, 60, 61, 62, 69, 74, 75, 76, 80, 83, 84, 108, 113, 114, 117, 118, 190, 191};
static const uint16_t HA_CAT_ENERGIE_IDX[] = {88, 89, 90, 91, 92, 93, 94, 95, 96, 131, 132, 139, 140, 141, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188};
static const uint16_t HA_CAT_KAMERA_IDX[] = {39, 85, 86, 87, 99, 100};
static const uint16_t HA_CAT_SKRIPTE_IDX[] = {119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130};
static const uint16_t* const HA_CAT_IDX[HA_CATEGORY_COUNT] = {
    HA_CAT_LICHT_IDX,
    HA_CAT_SCHALTER_IDX,
    HA_CAT_KLIMA_IDX,
    HA_CAT_SENSOR_IDX,
    HA_CAT_FENSTER_IDX,
    HA_CAT_ENERGIE_IDX,
    HA_CAT_KAMERA_IDX,
    HA_CAT_SKRIPTE_IDX,
};
static const uint16_t HA_CAT_CNT[HA_CATEGORY_COUNT] = {
    27,  // LICHT
    35,  // SCHALTER
    11,  // KLIMA
    28,  // SENSOR
    37,  // FENSTER
    38,  // ENERGIE
    6,  // KAMERA
    12,  // SKRIPTE
};
static const char* const HA_CAT_NAME[HA_CATEGORY_COUNT] = {
    "Licht",
    "Schalter",
    "Klima",
    "Sensor",
    "Fenster & Türen",
    "Energie",
    "Kameras",
    "Skripte",
};

// ─── Room indices ────────────────────────────────────────────────────────────
static const uint16_t HA_ROOM_DACHBODEN_IDX[] = {0, 1, 2, 3, 4, 5, 6};
static const uint16_t HA_ROOM_BAD_IDX[] = {7, 8, 9, 10, 11, 12, 13, 14};
static const uint16_t HA_ROOM_ANKLEIDEZIMMER_IDX[] = {15, 16, 17, 18, 19};
static const uint16_t HA_ROOM_BUERO_IDX[] = {20, 21, 22, 23, 24, 25};
static const uint16_t HA_ROOM_SCHLAFZIMMER_IDX[] = {26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
static const uint16_t HA_ROOM_TREPPE_IDX[] = {0};
static const uint16_t HA_ROOM_ERSTER_STOCK_IDX[] = {36};
static const uint16_t HA_ROOM_ABSTELLRAUM_IDX[] = {37, 38};
static const uint16_t HA_ROOM_FLUR_IDX[] = {39, 40, 41, 42, 43, 44, 45, 46};
static const uint16_t HA_ROOM_KUECHE_IDX[] = {47, 48, 49, 50, 51, 52, 53, 54, 55};
static const uint16_t HA_ROOM_WOHNZIMMER_IDX[] = {56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78};
static const uint16_t HA_ROOM_PANTRY_IDX[] = {79, 80};
static const uint16_t HA_ROOM_KLO_IDX[] = {81, 82, 83, 84};
static const uint16_t HA_ROOM_EINGANG_IDX[] = {85, 86};
static const uint16_t HA_ROOM_GARTEN_IDX[] = {87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98};
static const uint16_t HA_ROOM_EINFAHRT_IDX[] = {99, 100, 101, 102, 103, 104};
static const uint16_t HA_ROOM_KELLER_IDX[] = {105, 106, 107};
static const uint16_t HA_ROOM_HEIZUNG_IDX[] = {108};
static const uint16_t HA_ROOM_LABOR_IDX[] = {109, 110, 111, 112, 113, 114, 115, 116, 117, 118};
static const uint16_t HA_ROOM_CONTROLROOM_IDX[] = {119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188};
static const uint16_t HA_ROOM_WASCHKUECHE_IDX[] = {189, 190, 191, 192, 193};
static const uint16_t* const HA_ROOM_IDX[HA_ROOM_COUNT] = {
    HA_ROOM_DACHBODEN_IDX,
    HA_ROOM_BAD_IDX,
    HA_ROOM_ANKLEIDEZIMMER_IDX,
    HA_ROOM_BUERO_IDX,
    HA_ROOM_SCHLAFZIMMER_IDX,
    HA_ROOM_TREPPE_IDX,
    HA_ROOM_ERSTER_STOCK_IDX,
    HA_ROOM_ABSTELLRAUM_IDX,
    HA_ROOM_FLUR_IDX,
    HA_ROOM_KUECHE_IDX,
    HA_ROOM_WOHNZIMMER_IDX,
    HA_ROOM_PANTRY_IDX,
    HA_ROOM_KLO_IDX,
    HA_ROOM_EINGANG_IDX,
    HA_ROOM_GARTEN_IDX,
    HA_ROOM_EINFAHRT_IDX,
    HA_ROOM_KELLER_IDX,
    HA_ROOM_HEIZUNG_IDX,
    HA_ROOM_LABOR_IDX,
    HA_ROOM_CONTROLROOM_IDX,
    HA_ROOM_WASCHKUECHE_IDX,
};
static const uint16_t HA_ROOM_CNT[HA_ROOM_COUNT] = {
    7,  // DACHBODEN
    8,  // BAD
    5,  // ANKLEIDEZIMMER
    6,  // BUERO
    10,  // SCHLAFZIMMER
    0,  // TREPPE
    1,  // ERSTER_STOCK
    2,  // ABSTELLRAUM
    8,  // FLUR
    9,  // KUECHE
    23,  // WOHNZIMMER
    2,  // PANTRY
    4,  // KLO
    2,  // EINGANG
    12,  // GARTEN
    6,  // EINFAHRT
    3,  // KELLER
    1,  // HEIZUNG
    10,  // LABOR
    70,  // CONTROLROOM
    5,  // WASCHKUECHE
};
static const char* const HA_ROOM_NAME[HA_ROOM_COUNT] = {
    "Dachboden",
    "Bad",
    "Ankleidezimmer",
    "Büro",
    "Schlafzimmer",
    "Treppe",
    "1. OG",
    "Abstellraum",
    "Flur",
    "Küche",
    "Wohnzimmer",
    "Pantry",
    "Toilette",
    "Eingang",
    "Garten",
    "Einfahrt",
    "Keller",
    "Heizung",
    "Labor",
    "ControlRoom",
    "Waschküche",
};
static const bool HA_ROOM_VISIBLE[HA_ROOM_COUNT] = {
    true,  // DACHBODEN
    true,  // BAD
    true,  // ANKLEIDEZIMMER
    true,  // BUERO
    true,  // SCHLAFZIMMER
    true,  // TREPPE
    true,  // ERSTER_STOCK
    true,  // ABSTELLRAUM
    true,  // FLUR
    true,  // KUECHE
    true,  // WOHNZIMMER
    true,  // PANTRY
    true,  // KLO
    true,  // EINGANG
    true,  // GARTEN
    true,  // EINFAHRT
    true,  // KELLER
    true,  // HEIZUNG
    true,  // LABOR
    false,  // CONTROLROOM
    true,  // WASCHKUECHE
};

// ─── Summary template (for ha_rest: active counts per category then room) ─────
// Result: pipe-separated integers, 8 category counts + 21 room counts.
static const char HA_SUMMARY_TEMPLATE[] = "{%set l=['light.dach','light.bat_light','light.off_light_l1','light.sonoff_100135c1bc_1','light.smart_star_projector_background','light.smart_star_projector_laser','light.upf_light_ceiling','light.flo_light_ceiling','light.flo_ledstrip','light.kit_light_1','light.kit_light_buffet','light.kit_light_line','light.sonoff_10018427b3_1','light.wohnzimmerlufter','light.wohnzimmertv_backlight','light.liv_light_4','light.liv_light_1','light.liv_socket_1','light.liv_socket_2','light.toi_light','light.wled_2','light.run_wled_short','light.run_carport_wled','light.run_carport_wled_segment_1','light.bas_light_1','light.bas_light_2','light.lab_fan']%}{{l|select('is_state','on')|list|count}}|{%set s=['switch.sle_tv','switch.flo_siren_alarm','switch.liv_tv','switch.liv_mosquitokiller','switch.liv_sidematrix','switch.liv_backspeaker','switch.liv_usb_l1','switch.liv_usb_l2','switch.bas_heating','switch.lab_socket_desktop','switch.lab_socket_printer','switch.hsr01','switch.hsr03','switch.hsr05','switch.hsr06','switch.hsr04','switch.hsr02','switch.hsr10','switch.hsr11','switch.hsr14','switch.hsr13','switch.hsr12','switch.hsr17','switch.hsr19','switch.hsr15','switch.hsr18','switch.hsr09','switch.hsr08','switch.hsr16','switch.hsr07','switch.hsr20','switch.hsr21','switch.hsr22','switch.hsr24','switch.hsr23']%}{{s|select('is_state','on')|list|count}}|{%set k=['climate.bat_radiator_thermostat','climate.bat_thermostat','climate.dre_thermostat','climate.off_thermostat','climate.sle_thermostat','climate.flo_thermostat','climate.kit_thermostat','climate.liv_thermostat','climate.toi_thermostat','climate.lab_thermostat','climate.lau_thermostat']%}{{k|reject('is_state','off')|list|count}}|0|{%set w=['binary_sensor.roo_window_sensor_a_contact','binary_sensor.roo_window_sensor_c_contact','binary_sensor.roo_window_sensor_d_contact','binary_sensor.roo_window_sensor_b_contact','binary_sensor.bat_window_sensor_a_contact','binary_sensor.bat_window_sensor_b_contact','binary_sensor.wise_bat_upf_contact','binary_sensor.wise_dre_upf_contact','cover.off_shutter_window','binary_sensor.off_window_sensor_a_contact','binary_sensor.wise_off_upf_contact','binary_sensor.off_window_sensor_b_contact','cover.sle_shutter_window','binary_sensor.sle_window_sensor_contact','binary_sensor.wise_sle_upf_contact','binary_sensor.wise_cle_flo_contact','binary_sensor.flo_window_sensor_contact','cover.kit_shutter_window','binary_sensor.kit_window_sensor_contact','binary_sensor.wise_kit_flo_contact','cover.liv_shutter_patio_door','cover.liv_shutter_window_runway','cover.liv_shutter_window_garden','binary_sensor.wise_liv_flo_contact','binary_sensor.liv_window_sensor_a_contact','binary_sensor.liv_window_sensor_c_contact','binary_sensor.liv_window_sensor_b_contact','binary_sensor.wise_pan_kit_contact','binary_sensor.wise_toi_flo_contact','binary_sensor.toi_window_sensor_contact','binary_sensor.wise_hea_bas_contact','binary_sensor.lab_window_sensor_a_contact','binary_sensor.wise_lab_bas_contact','binary_sensor.lab_window_sensor_b_contact','binary_sensor.lab_window_sensor_c_contact','binary_sensor.wise_lau_bas_contact','binary_sensor.lau_window_sensor_contact']%}{{(w|select('is_state','on')|list|count)+(w|select('is_state','open')|list|count)+(w|select('is_state','opening')|list|count)}}|0|0|{%set p=['script.ambiente_bright_down','script.ambiente_bright_up','script.ambiente_lavendel','script.bedtime','script.eg_cover','script.fireplace_fire','script.fireplace_heat','script.fireplace_power','script.lrtoggleambiente','script.lrtoggletv','script.runway_light_off','script.runway_light_on']%}{{p|select('is_state','on')|list|count}}|{%for a in['dachboden','badezimmer','ankleidezimmer','buro','schlafzimmer','treppe','erster_stock','cleaning_room','flur','kuche','wohnzimmer','pantry','klo','eingang','garten','einfahrt','keller','heizungsraum','labor','controlroom','waschkuche']%}{{area_entities(a)|select('is_state','on')|list|count+area_entities(a)|select('is_state','open')|list|count}}|{%endfor%}";


// ─── Colours ────────────────────────────────────────────────────────────────
#define COL_BG       0x000000
#define COL_HDR      0x7D4CDB
#define COL_TAB      0x3D2B8C
#define COL_TILE_BG  0x1A2A3A
#define COL_TILE_SEL 0x5E35B1
#define COL_TILE_PRS 0x2A3A50
#define COL_TXT      0xFFFFFF
#define COL_TXT_DIM  0x888888
#define COL_TXT_SUB  0xFFFFFF
#define COL_ACCENT   0x7D4CDB

LV_FONT_DECLARE(ui_font_star);
LV_FONT_DECLARE(ui_font_ver14);
LV_FONT_DECLARE(ui_font_ver18);
LV_FONT_DECLARE(ui_font_ver24);
LV_FONT_DECLARE(ui_font_ver30);

static lv_style_t style_header_footer;
static lv_style_t style_button;
static lv_style_t style_button_pressed;
static lv_style_t style_button_checked;
static lv_style_t style_tabview;
static lv_style_t style_kb_main;
static lv_style_t style_kb_btn;
static lv_style_t style_kb_btn_pressed;
static lv_style_t style_kb_btn_disabled;

// ─── Static state ───────────────────────────────────────────────────────────
static bool s_showing_rooms = false;

// Header labels
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_date;
static lv_obj_t *s_lbl_sensors;

// Climate tab detail labels
static lv_obj_t *s_lbl_temp_in;
static lv_obj_t *s_lbl_hum_in;
static lv_obj_t *s_lbl_temp_out;
static lv_obj_t *s_lbl_hum_out;
static lv_obj_t *s_lbl_lux_detail;

// System tab
static lv_obj_t *s_lbl_radar_state;
static lv_obj_t *s_lbl_radar_detail;

// Sensor values (for strip update)
static float s_temp_in = 0, s_hum_in = 0;
static float s_temp_out = 0, s_hum_out = 0;
static float s_lux = 0;

// Home tab tile objects
#define MAX_TILES 25
static lv_obj_t *s_tile_cat_container = NULL;
static lv_obj_t *s_tile_rom_container = NULL;
static lv_obj_t *s_tiles[MAX_TILES]          = {};
static lv_obj_t *s_tile_state_lbls[MAX_TILES] = {};
static lv_obj_t *s_btn_cat   = NULL;
static lv_obj_t *s_btn_rooms = NULL;

// Overlay
static lv_obj_t *s_msgbox;
static lv_obj_t *s_msgbox_lbl;

// ─── Helpers ─────────────────────────────────────────────────────────────────

void apply_style(void)
{
    /* Tabview style */
    lv_style_init(&style_tabview);
    lv_style_set_bg_opa(&style_tabview, LV_OPA_TRANSP);
    lv_style_set_border_color(&style_tabview, (lv_color_t)lv_color_hex(0x9966CC));
    lv_style_set_border_width(&style_tabview, 0);
    lv_style_set_text_color(&style_tabview, (lv_color_t)lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_tabview, 0);

    /* Button base style */
    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, (lv_color_t)lv_color_hex(0xB19CD9));
    lv_style_set_bg_grad_color(&style_button, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_dir(&style_button, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_border_color(&style_button, (lv_color_t)lv_color_hex(0x9966CC));
    lv_style_set_border_width(&style_button, 1);
    lv_style_set_shadow_spread(&style_button, 2);
    lv_style_set_shadow_color(&style_button, (lv_color_t)lv_color_hex(0xFFFFFF));
    lv_style_set_text_color(&style_button, (lv_color_t)lv_color_hex(0xFFFFFF));

    /* Button pressed (separate style) */
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, (lv_color_t)lv_color_hex(0x8A6BC7));
    lv_style_set_bg_grad_color(&style_button_pressed, (lv_color_t)lv_color_hex(0x5B2C87));

    /* Button checked (separate style) */
    lv_style_init(&style_button_checked);
    lv_style_set_bg_color(&style_button_checked, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_color(&style_button_checked, (lv_color_t)lv_color_hex(0x4A1A5C));
    lv_style_set_text_color(&style_button_checked, (lv_color_t)lv_color_hex(0xFFF300));

    /* Header/Footer style */
    lv_style_init(&style_header_footer);
    lv_style_set_bg_color(&style_header_footer, (lv_color_t)lv_color_hex(0xB19CD9));
    lv_style_set_bg_grad_color(&style_header_footer, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_dir(&style_header_footer, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_header_footer, LV_OPA_COVER);
    lv_style_set_border_opa(&style_header_footer, LV_OPA_TRANSP);
    lv_style_set_radius(&style_header_footer, 0);
    lv_style_set_pad_all(&style_header_footer, 0);
    lv_style_set_pad_row(&style_header_footer, 0);
    lv_style_set_pad_column(&style_header_footer, 0);
    lv_style_set_border_color(&style_header_footer, (lv_color_t)lv_color_hex(0x9966CC));
    lv_style_set_text_color(&style_header_footer, (lv_color_t)lv_color_hex(0xFFFFFF));

    lv_style_init(&style_kb_main);
    lv_style_set_bg_opa(&style_kb_main, LV_OPA_TRANSP); // Hintergrund komplett transparent
    lv_style_set_border_width(&style_kb_main, 0);
    lv_style_set_pad_all(&style_kb_main, 10);

    lv_style_init(&style_kb_btn);
    lv_style_set_radius(&style_kb_btn, LV_RADIUS_CIRCLE); // runde Tasten
    lv_style_set_bg_color(&style_kb_btn, lv_color_white());
    lv_style_set_bg_opa(&style_kb_btn, LV_OPA_40); // halbtransparent
    lv_style_set_border_width(&style_kb_btn, 0);
    lv_style_set_text_color(&style_kb_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_kb_btn, &ui_font_star);
    lv_style_set_pad_all(&style_kb_btn, 5);
    lv_style_set_pad_gap(&style_kb_btn, 8); // Abstand zwischen Tasten

    lv_style_init(&style_kb_btn_pressed);
    lv_style_set_radius(&style_kb_btn_pressed, LV_RADIUS_CIRCLE); // runde Tasten
    lv_style_set_bg_color(&style_kb_btn_pressed, lv_color_hex(0x7D4CDB));
    lv_style_set_bg_opa(&style_kb_btn_pressed, LV_OPA_60); // weniger transparent bei gedrückt
    lv_style_set_border_width(&style_kb_btn_pressed, 0);
    lv_style_set_text_color(&style_kb_btn_pressed, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_kb_btn_pressed, &ui_font_star);
    lv_style_set_pad_all(&style_kb_btn_pressed, 5);
    lv_style_set_pad_gap(&style_kb_btn_pressed, 8); // Abstand zwischen Tasten

    lv_style_init(&style_kb_btn_disabled);
    lv_style_set_radius(&style_kb_btn_disabled, LV_RADIUS_CIRCLE); // runde Tasten
    lv_style_set_bg_color(&style_kb_btn_disabled, lv_color_white());
    lv_style_set_bg_opa(&style_kb_btn_disabled, LV_OPA_20); // halbtransparent
    lv_style_set_border_width(&style_kb_btn_disabled, 0);
    lv_style_set_text_color(&style_kb_btn_disabled, lv_color_hex(0x333333));
    lv_style_set_text_font(&style_kb_btn_disabled, &ui_font_star);
    lv_style_set_pad_all(&style_kb_btn_disabled, 5);
    lv_style_set_pad_gap(&style_kb_btn_disabled, 8);

    /* Apply global background color to active screen */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, (lv_color_t)lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* Add tabview main style to screen so tabview children can inherit if desired */
    lv_obj_add_style(scr, &style_tabview, LV_PART_MAIN);
}

static void update_sensor_strip()
{
    char buf[80];
    snprintf(buf, sizeof(buf),
        "In %.1f\xC2\xB0""C %.0f%%  Out %.1f\xC2\xB0""C %.0f%%  %.0f lx",
        s_temp_in, s_hum_in, s_temp_out, s_hum_out, s_lux);
    lv_label_set_text(s_lbl_sensors, buf);
}

// ─── Tile interaction ────────────────────────────────────────────────────────
static void tile_event_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    int bound = s_showing_rooms ? HA_ROOM_COUNT : HA_CATEGORY_COUNT;
    for (int i = 0; i < bound && i < MAX_TILES; i++) {
        if (!s_tiles[i]) continue;
        lv_obj_set_style_bg_color(s_tiles[i],
            i == idx ? lv_color_hex(COL_TILE_SEL) : lv_color_hex(COL_TILE_BG), 0);
    }
}

static void build_tiles(lv_obj_t *container)
{
    lv_obj_clean(container);
    memset(s_tiles, 0, sizeof(s_tiles));
    memset(s_tile_state_lbls, 0, sizeof(s_tile_state_lbls));

    int count = s_showing_rooms ? HA_ROOM_COUNT : HA_CATEGORY_COUNT;
    int cols  = s_showing_rooms ? 5 : 4;
    int rows  = s_showing_rooms ? 4 : 2;
    int gap   = 4;
    int tile_w = (DISPLAY_WIDTH - 42 - (cols - 1) * gap) / cols;
    int tile_h = (330           -  8 - (rows - 1) * gap) / rows;

    for (int i = 0; i < count && i < MAX_TILES; i++) {
        // Skip hidden rooms (entities still exist in the system)
        if (s_showing_rooms && !HA_ROOM_VISIBLE[i]) continue;

        lv_obj_t *tile = lv_obj_create(container);
        lv_obj_set_size(tile, tile_w, tile_h);
        lv_obj_set_style_bg_color(tile, lv_color_hex(COL_TILE_BG), 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(COL_TILE_PRS), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(tile, 8, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 6, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        // Pass real index i so CMD_SHOW_GROUP uses the correct room index
        lv_obj_add_event_cb(tile, tile_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        const char *name = s_showing_rooms ? HA_ROOM_NAME[i] : HA_CAT_NAME[i];
        uint16_t    cnt  = s_showing_rooms ? HA_ROOM_CNT[i]  : HA_CAT_CNT[i];

        lv_obj_t *name_lbl = lv_label_create(tile);
        lv_label_set_text(name_lbl, name);
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name_lbl, tile_w - 12);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(COL_TXT), 0);
        lv_obj_set_style_text_font(name_lbl,
            s_showing_rooms ? &ui_font_ver14 : &ui_font_ver18, 0);
        lv_obj_align(name_lbl, LV_ALIGN_TOP_MID, 0, 4);

        // Count label (static)
        char cnt_buf[16];
        snprintf(cnt_buf, sizeof(cnt_buf), "%u", cnt);
        lv_obj_t *cnt_lbl = lv_label_create(tile);
        lv_label_set_text(cnt_lbl, cnt_buf);
        lv_obj_set_style_text_color(cnt_lbl, lv_color_hex(COL_TXT_DIM), 0);
        lv_obj_set_style_text_font(cnt_lbl, &ui_font_ver14, 0);
        lv_obj_align(cnt_lbl, LV_ALIGN_CENTER, 0, 2);

        // State label (updated via ha_rest)
        lv_obj_t *state_lbl = lv_label_create(tile);
        lv_label_set_text(state_lbl, "...");
        lv_obj_set_style_text_color(state_lbl, lv_color_hex(COL_ACCENT), 0);
        lv_obj_set_style_text_font(state_lbl, &ui_font_ver14, 0);
        lv_obj_align(state_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

        s_tiles[i]           = tile;
        s_tile_state_lbls[i] = state_lbl;
    }
}

// ─── Overlay ─────────────────────────────────────────────────────────────────
static void build_msgbox(lv_obj_t *parent)
{
    s_msgbox = lv_obj_create(parent);
    lv_obj_set_size(s_msgbox, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(s_msgbox, 0, 0);
    lv_obj_set_style_bg_color(s_msgbox, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_msgbox, LV_OPA_80, 0);
    lv_obj_set_style_border_width(s_msgbox, 0, 0);
    lv_obj_set_style_radius(s_msgbox, 0, 0);
    lv_obj_clear_flag(s_msgbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_msgbox, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *box = lv_obj_create(s_msgbox);
    lv_obj_set_size(box, 500, 160);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x1A2A3A), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(COL_ACCENT), 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);

    s_msgbox_lbl = lv_label_create(box);
    lv_label_set_text(s_msgbox_lbl, "");
    lv_label_set_long_mode(s_msgbox_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_msgbox_lbl, 460);
    lv_obj_set_style_text_color(s_msgbox_lbl, lv_color_hex(COL_TXT), 0);
    lv_obj_set_style_text_font(s_msgbox_lbl, &ui_font_ver18, 0);
    lv_obj_set_style_text_align(s_msgbox_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_msgbox_lbl, LV_ALIGN_CENTER, 0, 0);
}

static void stab0_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 0, LV_ANIM_OFF);
    s_showing_rooms = false;
    build_tiles(s_tile_cat_container);
}
static void stab1_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 1, LV_ANIM_OFF);
    s_showing_rooms = true;
    build_tiles(s_tile_rom_container);
}
static void stab2_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 2, LV_ANIM_OFF);
}

// Horizontal icon button for the inner settings tab bar
static lv_obj_t *make_inner_tab_btn(lv_obj_t *parent, const lv_img_dsc_t *icon,
                                     lv_event_cb_t cb, lv_obj_t *tv)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, 50);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_t *img = lv_img_create(btn);
    lv_img_set_src(img, icon);
    lv_obj_set_style_img_recolor(img, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(img);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, tv);
    lv_obj_add_style(btn, &style_button, LV_PART_MAIN);
    lv_obj_add_style(btn, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    return btn;
}

bool osiris_top_init(void)
{
    apply_style();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(scr, &style_header_footer, LV_PART_MAIN);

    // ── Header ──────────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 44);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_add_style(hdr, &style_header_footer, LV_PART_MAIN);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 4, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_unit = lv_label_create(hdr);
    lv_label_set_text(lbl_unit, "OSIRIS");
    lv_obj_set_style_text_color(lbl_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_unit, &ui_font_ver14, 0);
    lv_obj_align(lbl_unit, LV_ALIGN_TOP_LEFT, 0, 0);

    s_lbl_sensors = lv_label_create(hdr);
    lv_label_set_text(s_lbl_sensors, "-- \xC2\xB0""C --%  -- \xC2\xB0""C --%  -- lx");
    lv_obj_set_style_text_color(s_lbl_sensors, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lbl_sensors, &ui_font_ver14, 0);
    lv_obj_align(s_lbl_sensors, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    s_lbl_time = lv_label_create(hdr);
    lv_label_set_text(s_lbl_time, "--:--");
    lv_obj_set_style_text_color(s_lbl_time, lv_color_hex(COL_TXT), 0);
    lv_obj_set_style_text_font(s_lbl_time, &ui_font_ver30, 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_CENTER, 0, 0);

    s_lbl_date = lv_label_create(hdr);
    lv_label_set_text(s_lbl_date, "--.--.----");
    lv_obj_set_style_text_color(s_lbl_date, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lbl_date, &ui_font_ver14, 0);
    lv_obj_align(s_lbl_date, LV_ALIGN_RIGHT_MID, -4, 0);

    // ── Tab view ────────────────────────────────────────────────────────────
    lv_obj_t *tv = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_LEFT);
    lv_tabview_set_tab_bar_size(tv, 50);
    lv_obj_set_pos(tv, 0, 44);
    lv_obj_set_size(tv, DISPLAY_WIDTH, DISPLAY_HEIGHT - 44);
    //lv_obj_set_style_bg_color(tv, lv_color_hex(COL_BG), 0);
    //lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, 0);
    lv_obj_add_style(tv, &style_header_footer, LV_PART_MAIN);
    lv_obj_add_style(tv, &style_tabview, LV_PART_MAIN);

    lv_obj_t *tab_cat = lv_tabview_add_tab(tv, "Categories");
    s_tile_cat_container = lv_obj_create(tab_cat);
    lv_obj_set_size(s_tile_cat_container, DISPLAY_WIDTH, 386);
    lv_obj_set_style_bg_opa(s_tile_cat_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_tile_cat_container, 0, 0);
    lv_obj_set_style_pad_all(s_tile_cat_container, 4, 0);
    lv_obj_set_style_pad_gap(s_tile_cat_container, 4, 0);
    lv_obj_align(s_tile_cat_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_layout(s_tile_cat_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_tile_cat_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_tile_cat_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    build_tiles(s_tile_cat_container);
    
    lv_obj_t *tab_rom = lv_tabview_add_tab(tv, "Rooms");
    s_tile_rom_container = lv_obj_create(tab_rom);
    lv_obj_set_size(s_tile_rom_container, DISPLAY_WIDTH, 386);
    lv_obj_set_style_bg_opa(s_tile_rom_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_tile_rom_container, 0, 0);
    lv_obj_set_style_pad_all(s_tile_rom_container, 4, 0);
    lv_obj_set_style_pad_gap(s_tile_rom_container, 4, 0);
    lv_obj_align(s_tile_rom_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_layout(s_tile_rom_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_tile_rom_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_tile_rom_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *tab_set = lv_tabview_add_tab(tv, "Settings");



    // Hide native text tab buttons — replaced with icon bar below
    lv_obj_t *native_btns = lv_tabview_get_tab_btns(tv);
    lv_obj_add_flag(native_btns, LV_OBJ_FLAG_HIDDEN);

    // Floating icon bar in the space reserved by LV_DIR_BOTTOM, 50
    lv_obj_t *bar = lv_obj_create(tv);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, LV_PCT(100), 50);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(bar, &style_header_footer, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_clear_flag(lv_tabview_get_content(tv), LV_OBJ_FLAG_SCROLLABLE);

    make_inner_tab_btn(bar, &categories, stab0_cb, tv);
    make_inner_tab_btn(bar, &rooms,      stab1_cb, tv);
    make_inner_tab_btn(bar, &settings,   stab2_cb, tv);

    build_msgbox(lv_layer_top());
    
    fprintf(stderr, "[horus] screen built OK\n");
    return true;
}

void osiris_top_tick(void)
{
    
}
