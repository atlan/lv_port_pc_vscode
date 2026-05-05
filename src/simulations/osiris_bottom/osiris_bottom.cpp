#include "osiris_bottom.h"
#include "src/glyphs/osiris_icons/osiris_icons.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "lvgl/lvgl.h"

#define DISPLAY_WIDTH  800
#define DISPLAY_HEIGHT 480


// ─── Enums ───────────────────────────────────────────────────────────────────
enum HaDomain : uint8_t {
    HaD_Light=0, HaD_Switch, HaD_Climate, HaD_Sensor, HaD_BinarySensor, HaD_Cover, HaD_Camera, HaD_Script, HaD_Fan
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
    HaSvc_FanToggle,   // fan with optional percentage (0-100)
    HaSvc_None         // read-only sensor/binary_sensor/climate/camera
};

struct HaEntity {
    const char* entity_id;
    const char* name;
    HaDomain    domain;
    HaService   service;
};

#define HA_ENTITY_COUNT 194
static const HaEntity HA_ENTITIES[] = {
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
    {"light.run_wled_short", "light.run_wled_short", HaD_Light, HaSvc_DimToggle},
    {"light.wled_2", "Run LEDStripe", HaD_Light, HaSvc_DimToggle},
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
    {"script.bedtime", "Bedtime ", HaD_Script, HaSvc_ScriptRun},
    {"script.eg_cover", "EG Cover", HaD_Script, HaSvc_ScriptRun},
    {"script.lrtoggletv", "LRToggleTV", HaD_Script, HaSvc_ScriptRun},
    {"script.lrtoggleambiente", "LRToggleAmbiente", HaD_Script, HaSvc_ScriptRun},
    {"script.ambiente_bright_up", "Ambiente Bright Up", HaD_Script, HaSvc_ScriptRun},
    {"script.ambiente_bright_down", "Ambiente Bright Down", HaD_Script, HaSvc_ScriptRun},
    {"script.fireplace_power", "Fireplace Power", HaD_Script, HaSvc_ScriptRun},
    {"script.fireplace_fire", "Fireplace Fire", HaD_Script, HaSvc_ScriptRun},
    {"script.fireplace_heat", "Fireplace Heat", HaD_Script, HaSvc_ScriptRun},
    {"script.ambiente_lavendel", "Ambiente Lavendel ", HaD_Script, HaSvc_ScriptRun},
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

// ─── Fav-for-control indices (Bottom default tab tiles) ───────────────────────
static const uint16_t HA_FAV_IDX[] = {56, 119, 120, 121, 122, 50, 41, 105, 36};
#define HA_FAV_CNT 9

// ─── Summary template (for ha_rest: active counts per category then room) ─────
// Result: pipe-separated integers, 8 category counts + 21 room counts.
static const char HA_SUMMARY_TEMPLATE[] = "{%set l=['light.dach','light.bat_light','light.off_light_l1','light.sonoff_100135c1bc_1','light.smart_star_projector_background','light.smart_star_projector_laser','light.upf_light_ceiling','light.flo_light_ceiling','light.flo_ledstrip','light.kit_light_1','light.kit_light_buffet','light.kit_light_line','light.sonoff_10018427b3_1','light.wohnzimmerlufter','light.wohnzimmertv_backlight','light.liv_light_4','light.liv_light_1','light.liv_socket_1','light.liv_socket_2','light.toi_light','light.run_wled_short','light.wled_2','light.run_carport_wled','light.run_carport_wled_segment_1','light.bas_light_1','light.bas_light_2','light.lab_fan']%}{{l|select('is_state','on')|list|count}}|{%set s=['switch.sle_tv','switch.flo_siren_alarm','switch.liv_tv','switch.liv_mosquitokiller','switch.liv_sidematrix','switch.liv_backspeaker','switch.liv_usb_l1','switch.liv_usb_l2','switch.bas_heating','switch.lab_socket_desktop','switch.lab_socket_printer','switch.hsr01','switch.hsr03','switch.hsr05','switch.hsr06','switch.hsr04','switch.hsr02','switch.hsr10','switch.hsr11','switch.hsr14','switch.hsr13','switch.hsr12','switch.hsr17','switch.hsr19','switch.hsr15','switch.hsr18','switch.hsr09','switch.hsr08','switch.hsr16','switch.hsr07','switch.hsr20','switch.hsr21','switch.hsr22','switch.hsr24','switch.hsr23']%}{{s|select('is_state','on')|list|count}}|{%set k=['climate.bat_radiator_thermostat','climate.bat_thermostat','climate.dre_thermostat','climate.off_thermostat','climate.sle_thermostat','climate.flo_thermostat','climate.kit_thermostat','climate.liv_thermostat','climate.toi_thermostat','climate.lab_thermostat','climate.lau_thermostat']%}{{k|reject('is_state','off')|list|count}}|0|{%set w=['binary_sensor.roo_window_sensor_a_contact','binary_sensor.roo_window_sensor_c_contact','binary_sensor.roo_window_sensor_d_contact','binary_sensor.roo_window_sensor_b_contact','binary_sensor.bat_window_sensor_a_contact','binary_sensor.bat_window_sensor_b_contact','binary_sensor.wise_bat_upf_contact','binary_sensor.wise_dre_upf_contact','cover.off_shutter_window','binary_sensor.off_window_sensor_a_contact','binary_sensor.wise_off_upf_contact','binary_sensor.off_window_sensor_b_contact','cover.sle_shutter_window','binary_sensor.sle_window_sensor_contact','binary_sensor.wise_sle_upf_contact','binary_sensor.wise_cle_flo_contact','binary_sensor.flo_window_sensor_contact','cover.kit_shutter_window','binary_sensor.kit_window_sensor_contact','binary_sensor.wise_kit_flo_contact','cover.liv_shutter_patio_door','cover.liv_shutter_window_runway','cover.liv_shutter_window_garden','binary_sensor.wise_liv_flo_contact','binary_sensor.liv_window_sensor_a_contact','binary_sensor.liv_window_sensor_c_contact','binary_sensor.liv_window_sensor_b_contact','binary_sensor.wise_pan_kit_contact','binary_sensor.wise_toi_flo_contact','binary_sensor.toi_window_sensor_contact','binary_sensor.wise_hea_bas_contact','binary_sensor.lab_window_sensor_a_contact','binary_sensor.wise_lab_bas_contact','binary_sensor.lab_window_sensor_b_contact','binary_sensor.lab_window_sensor_c_contact','binary_sensor.wise_lau_bas_contact','binary_sensor.lau_window_sensor_contact']%}{{(w|select('is_state','on')|list|count)+(w|select('is_state','open')|list|count)+(w|select('is_state','opening')|list|count)}}|0|0|{%set p=['script.bedtime','script.eg_cover','script.lrtoggletv','script.lrtoggleambiente','script.ambiente_bright_up','script.ambiente_bright_down','script.fireplace_power','script.fireplace_fire','script.fireplace_heat','script.ambiente_lavendel','script.runway_light_off','script.runway_light_on']%}{{p|select('is_state','on')|list|count}}|{%for a in['dachboden','badezimmer','ankleidezimmer','buro','schlafzimmer','treppe','erster_stock','cleaning_room','flur','kuche','wohnzimmer','pantry','klo','eingang','garten','einfahrt','keller','heizungsraum','labor','controlroom','waschkuche']%}{{area_entities(a)|select('is_state','on')|list|count+area_entities(a)|select('is_state','open')|list|count}}|{%endfor%}";



// ─── Colours ────────────────────────────────────────────────────────────────
#define COL_BG       0x000000
#define COL_TILE_BG  0x1A2A3A
#define COL_TILE_SEL 0x5E35B1
#define COL_TILE_PRS 0x2A3A50
#define COL_TXT      0xFFFFFF
#define COL_TXT_DIM  0x888888
#define COL_ACCENT   0x7D4CDB

LV_FONT_DECLARE(ui_font_ver14);
LV_FONT_DECLARE(ui_font_ver18);
LV_FONT_DECLARE(ui_font_ver18b);
LV_FONT_DECLARE(ui_font_ver24);
LV_FONT_DECLARE(ui_font_ver30);
LV_FONT_DECLARE(ui_font_star);

static lv_style_t style_header_footer;
static lv_style_t style_button;
static lv_style_t style_button_pressed;
static lv_style_t style_button_checked;
static lv_style_t style_tabview;

#define MAX_GROUP_ROWS 100

// Internal-only action codes (never leave this file)
enum : uint8_t {
    ACT_TEMP_UP      = 6, ACT_TEMP_DN   = 7,   // climate setpoint
    ACT_BRIGHT_UP    = 8, ACT_BRIGHT_DN = 9,   // light brightness
    ACT_CAMERA_SHOW  = 10,                       // open camera stream overlay
    ACT_FAN_UP       = 11, ACT_FAN_DN  = 12,   // fan percentage
};

// ─── Tabs: 0=Uhr  1=Steuerung  2=Temp  3=Energie  4=Bat ─────────────────────
static lv_obj_t *s_tv;
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_date;
static lv_obj_t *s_lbl_lux;
static lv_obj_t *s_lbl_presence;
static lv_obj_t *s_tab_steuerung;

// ─── Fav-for-control tiles (Uhr tab) ─────────────────────────────────────────
// Max 8 tiles: interleaved left/right (i%2), 4 rows per column.
#define FAV_TILE_MAX 8
static lv_obj_t *s_fav_tile[FAV_TILE_MAX]      = {};
static lv_obj_t *s_fav_lbl_state[FAV_TILE_MAX] = {};
static lv_obj_t *s_fav_lbl_unit[FAV_TILE_MAX]  = {};
static char      s_fav_raw[FAV_TILE_MAX][16]   = {};

// ─── Temp chart state ────────────────────────────────────────────────────────
#define TEMP_CHART_MAX 580
static lv_obj_t          *s_tc          = nullptr;
static lv_chart_series_t *s_tc_out      = nullptr;
static lv_chart_series_t *s_tc_liv      = nullptr;
static lv_chart_series_t *s_tc_lab      = nullptr;
static lv_obj_t          *s_tc_tlbl[3]  = {};  // X-axis time labels: left/mid/right

// ─── Energie chart state ─────────────────────────────────────────────────────
#define ENERGY_CHART_MAX 290
static lv_obj_t          *s_bc          = nullptr;  // battery SoC chart (Bat tab)
static lv_chart_series_t *s_bc_bat      = nullptr;
static lv_obj_t          *s_bc_tlbl[3]  = {};
static lv_obj_t          *s_pc          = nullptr;  // power chart (Energie tab)
static lv_chart_series_t *s_pc_tas      = nullptr;
static lv_chart_series_t *s_pc_out      = nullptr;
static lv_chart_series_t *s_pc_s1       = nullptr;
static lv_chart_series_t *s_pc_s2       = nullptr;
static lv_chart_series_t *s_pc_s3       = nullptr;
static lv_chart_series_t *s_pc_s4       = nullptr;
static lv_obj_t          *s_ec_tlbl[3]  = {};

enum HaAction : uint8_t {
    HA_TURN_ON              = 0,
    HA_TURN_OFF             = 1,
    HA_COVER_OPEN           = 2,
    HA_COVER_CLOSE          = 3,
    HA_COVER_STOP           = 4,
    HA_CLIMATE_SET_TEMP     = 5,
    HA_LIGHT_SET_BRIGHTNESS = 6,  // param = brightness 0-255
    HA_FAN_SET_PERCENTAGE   = 7,  // param = percentage 0-100
};

typedef void (*ui_bottom_action_cb_t)(uint16_t entity_idx, HaAction action, float param);

struct EntityRow {
    uint16_t  entity_idx;
    HaDomain  domain;
    lv_obj_t *lbl_state;
    lv_obj_t *lbl_extra;    // climate: "cur/set" label; nullptr otherwise
    float     current_temp; // climate: last known current temp
    float     setpoint;     // climate: current target temp for +/-
};
static EntityRow             s_rows[MAX_GROUP_ROWS];
static uint16_t              s_row_count   = 0;
static ui_bottom_action_cb_t s_action_cb   = nullptr;
static ui_bottom_action_cb_t s_fav_action_cb = nullptr;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static void apply_style(void)
{
    lv_style_init(&style_tabview);
    lv_style_set_bg_opa(&style_tabview, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_tabview, 0);
    lv_style_set_text_color(&style_tabview, (lv_color_t)lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_tabview, 0);

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

    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, (lv_color_t)lv_color_hex(0x8A6BC7));
    lv_style_set_bg_grad_color(&style_button_pressed, (lv_color_t)lv_color_hex(0x5B2C87));

    lv_style_init(&style_button_checked);
    lv_style_set_bg_color(&style_button_checked, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_color(&style_button_checked, (lv_color_t)lv_color_hex(0x4A1A5C));
    lv_style_set_text_color(&style_button_checked, (lv_color_t)lv_color_hex(0xFFF300));

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
    lv_style_set_text_color(&style_header_footer, (lv_color_t)lv_color_hex(0xFFFFFF));
}

// ─── State helpers ────────────────────────────────────────────────────────────
static const char *fmt_state(const char *raw)
{
    if (!raw || !raw[0])              return "?";
    if (strcmp(raw,"on")        == 0) return "an";
    if (strcmp(raw,"off")       == 0) return "aus";
    if (strcmp(raw,"open")      == 0) return "auf";
    if (strcmp(raw,"closed")    == 0) return "zu";
    if (strcmp(raw,"opening")   == 0) return "\xc3\xb6""ffnet";
    if (strcmp(raw,"closing")   == 0) return "schlie\xc3\x9f""t";
    if (strcmp(raw,"heat")      == 0) return "heizen";
    if (strcmp(raw,"cool")      == 0) return "k\xc3\xbc""hlen";
    if (strcmp(raw,"auto")      == 0) return "auto";
    if (strcmp(raw,"idle")      == 0) return "bereit";
    if (strcmp(raw,"recording") == 0) return "aufnehmen";
    if (strcmp(raw,"unavailable") == 0) return "n/v";
    if (strcmp(raw,"unknown")   == 0) return "?";
    return raw;
}

static lv_color_t state_color(const char *raw)
{
    if (!raw || !raw[0]) return lv_color_hex(0x666688);
    if (strcmp(raw,"on") == 0 || strcmp(raw,"open") == 0 ||
        strcmp(raw,"heat") == 0 || strcmp(raw,"cool") == 0 || strcmp(raw,"auto") == 0)
        return lv_color_hex(0x44BB66);
    if (strcmp(raw,"off") == 0 || strcmp(raw,"closed") == 0)
        return lv_color_hex(0x666688);
    if (strcmp(raw,"unavailable") == 0 || strcmp(raw,"unknown") == 0)
        return lv_color_hex(0x555555);
    return lv_color_hex(0xAAAACC);
}

// ─── Action button callback ───────────────────────────────────────────────────
static void action_btn_cb(lv_event_t *) { /* simulator: no-op */ }
#if HA_FAV_CNT > 0
static void fav_tile_cb(lv_event_t *e);
#endif

// ─── Button / label factories ─────────────────────────────────────────────────
// user_data encodes (entity_idx << 8 | action_code) so the callback can
// decode both values from a single pointer.
static lv_obj_t *make_btn(lv_obj_t *parent, const char *text,
                           int w, uint16_t eidx, uint8_t act,
                           uint32_t bg = 0x7D4CDB)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, 36);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x5E35B1), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_user_data(btn, (void *)(uintptr_t)((uint32_t)eidx << 8 | act));
    lv_obj_add_event_cb(btn, action_btn_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &ui_font_ver14, 0);
    lv_obj_center(lbl);
    return btn;
}

static lv_obj_t *make_state_lbl(lv_obj_t *parent, int w, const char *raw)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, fmt_state(raw));
    lv_obj_set_width(lbl, w);
    lv_obj_set_height(lbl, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(lbl, state_color(raw), 0);
    lv_obj_set_style_text_font(lbl, &ui_font_ver14, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    return lbl;
}

// ─── Build Tab "Uhr" ──────────────────────────────────────────────────────────
static void build_tab_default(lv_obj_t *tab)
{
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(tab, &style_header_footer, LV_PART_MAIN);

    s_lbl_time = lv_label_create(tab);
    lv_label_set_text(s_lbl_time, "--:--");
    lv_obj_set_style_text_color(s_lbl_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_lbl_time, &ui_font_star, 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_CENTER, 0, -30);

    s_lbl_date = lv_label_create(tab);
    lv_label_set_text(s_lbl_date, "--.--.----");
    lv_obj_set_style_text_color(s_lbl_date, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lbl_date, &ui_font_ver24, 0);
    lv_obj_align(s_lbl_date, LV_ALIGN_CENTER, 0, 30);

#if HA_FAV_CNT > 0
    const int FAV_TW  = 185;
    const int FAV_TH  = 100;
    const int FAV_GAP = 5;
    const int FAV_X[2] = { 5, DISPLAY_WIDTH - FAV_TW - 5 };  // left=5, right=610
    const int n = (HA_FAV_CNT < FAV_TILE_MAX) ? HA_FAV_CNT : FAV_TILE_MAX;

    for (int i = 0; i < n; i++) {
        int col = i % 2;                               // 0=left, 1=right (interleaved)
        int row = i / 2;                               // 0..3
        int x   = FAV_X[col];
        int y   = FAV_GAP + row * (FAV_TH + FAV_GAP);

        lv_obj_t *tile = lv_obj_create(tab);
        lv_obj_set_size(tile, FAV_TW, FAV_TH);
        lv_obj_set_pos(tile, x, y);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        //lv_obj_set_style_bg_color(tile, lv_color_hex(0x0D1B2A), 0);
        //lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        //lv_obj_set_style_border_color(tile, lv_color_hex(0x3A4A6A), 0);
        //lv_obj_set_style_border_width(tile, 1, 0);
        //lv_obj_set_style_radius(tile, 6, 0);
        lv_obj_set_style_pad_all(tile, 6, 0);
        lv_obj_set_style_pad_row(tile, 0, 0);
        lv_obj_set_style_pad_column(tile, 0, 0);
        //lv_obj_set_style_bg_color(tile, lv_color_hex(0x1E2D3E), LV_STATE_PRESSED);
        lv_obj_set_user_data(tile, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(tile, fav_tile_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_style(tile, &style_button, LV_PART_MAIN);
        lv_obj_add_style(tile, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
        
        s_fav_tile[i] = tile;

        // Entity name — static label from HA_ENTITIES at build time
        uint16_t eidx = HA_FAV_IDX[i];
        const char *ename = (eidx < HA_ENTITY_COUNT) ? HA_ENTITIES[eidx].name : "?";
        lv_obj_t *ln = lv_label_create(tile);
        lv_label_set_text(ln, ename);
        lv_label_set_long_mode(ln, LV_LABEL_LONG_DOT);
        lv_obj_set_width(ln, FAV_TW - 12);
        lv_obj_set_height(ln, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(ln, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(ln, &ui_font_ver18, 0);
        lv_obj_align(ln, LV_ALIGN_TOP_MID, 0, 0);

        // State — updated at runtime via ui_bottom_update_fav_tiles
        lv_obj_t *ls = lv_label_create(tile);
        lv_label_set_text(ls, "--");
        lv_obj_set_style_text_font(ls, &ui_font_ver18, 0);
        lv_obj_set_style_text_color(ls, lv_color_hex(0xCCCCCC), 0);
        lv_obj_align(ls, LV_ALIGN_CENTER, 0, 0);
        s_fav_lbl_state[i] = ls;

        // Unit / extra — updated at runtime
        lv_obj_t *lu = lv_label_create(tile);
        lv_label_set_text(lu, "");
        lv_obj_set_style_text_color(lu, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_text_font(lu, &ui_font_ver14, 0);
        lv_obj_align(lu, LV_ALIGN_BOTTOM_MID, 0, 0);
        s_fav_lbl_unit[i] = lu;
    }
#endif
}

// ─── Fav tile click handler ───────────────────────────────────────────────────
#if HA_FAV_CNT > 0
static void fav_tile_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!s_fav_action_cb) return;

    lv_obj_t *tile = (lv_obj_t *)lv_event_get_target(e);
    int tidx = (int)(uintptr_t)lv_obj_get_user_data(tile);
    if (tidx < 0 || tidx >= FAV_TILE_MAX) return;

    const int n = (HA_FAV_CNT < FAV_TILE_MAX) ? HA_FAV_CNT : FAV_TILE_MAX;
    if (tidx >= n) return;

    uint16_t eidx = HA_FAV_IDX[tidx];
    if (eidx >= HA_ENTITY_COUNT) return;

    HaDomain    dom = HA_ENTITIES[eidx].domain;
    const char *raw = s_fav_raw[tidx];

    HaAction    act;
    const char *opt = nullptr;

    switch (dom) {
    case HaD_Light:
    case HaD_Switch:
    case HaD_Fan:
        act = (strcmp(raw, "on") == 0) ? HA_TURN_OFF : HA_TURN_ON;
        opt = (act == HA_TURN_ON) ? "on" : "off";
        break;
    case HaD_Script:
        act = HA_TURN_ON;
        break;
    case HaD_Cover:
        if (strcmp(raw, "open") == 0 || strcmp(raw, "opening") == 0) {
            act = HA_COVER_CLOSE;
            opt = "closing";
        } else {
            act = HA_COVER_OPEN;
            opt = "opening";
        }
        break;
    default:
        return;
    }

    if (opt && s_fav_lbl_state[tidx]) {
        strncpy(s_fav_raw[tidx], opt, sizeof(s_fav_raw[0]) - 1);
        s_fav_raw[tidx][sizeof(s_fav_raw[0]) - 1] = '\0';
        lv_label_set_text(s_fav_lbl_state[tidx], fmt_state(opt));
        lv_obj_set_style_text_color(s_fav_lbl_state[tidx], state_color(opt), 0);
    }

    s_fav_action_cb(eidx, act, 0.0f);
}
#endif

// ─── Build Tab "Steuerung" skeleton ──────────────────────────────────────────
static void build_tab_steuerung(lv_obj_t *tab)
{
    s_tab_steuerung = tab;
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(tab, LV_GRAD_DIR_NONE, 0);
    lv_obj_set_style_pad_all(tab, 0, 0);
    lv_obj_set_style_pad_row(tab, 0, 0);
    lv_obj_set_style_pad_column(tab, 0, 0);
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *hint = lv_label_create(tab);
    lv_label_set_text(hint, "Keine Gruppe ausgew\xc3\xa4""hlt");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(hint, &ui_font_ver18, 0);
    lv_obj_set_style_pad_all(hint, 16, 0);
}

// ─── Chart helper: create a styled chart object ───────────────────────────────
static lv_obj_t *make_chart(lv_obj_t *parent, int w, int h, int x, int y,
                             int32_t ymin, int32_t ymax,
                             uint8_t hdiv, uint8_t vdiv)
{
    lv_obj_t *c = lv_chart_create(parent);
    lv_obj_set_size(c, w, h);
    lv_obj_set_pos(c, x, y);
    lv_chart_set_type(c, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(c, 1);
    lv_chart_set_range(c, LV_CHART_AXIS_PRIMARY_Y, ymin, ymax);
    lv_chart_set_div_line_count(c, hdiv, vdiv);
    lv_obj_set_style_width(c, 0, LV_PART_INDICATOR);
    lv_obj_set_style_height(c, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(c, 2, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(c, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_line_color(c, lv_color_hex(0x1A2A3A), 0);
    lv_obj_set_style_pad_all(c, 4, 0);
    return c;
}

// Y-axis tick label formatting removed (LVGL v9: draw-part API gone)

// ─── X-axis time label row ───────────────────────────────────────────────────
// left_pad: align with chart plot area
static void make_time_row(lv_obj_t *tab, int y, int h, lv_obj_t **lbls, int left_pad = 6)
{
    lv_obj_t *row = lv_obj_create(tab);
    lv_obj_set_size(row, DISPLAY_WIDTH, h);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_left(row, left_pad, 0);
    lv_obj_set_style_pad_right(row, 6, 0);
    lv_obj_set_style_pad_ver(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    const lv_text_align_t align[3] = { LV_TEXT_ALIGN_LEFT, LV_TEXT_ALIGN_CENTER, LV_TEXT_ALIGN_RIGHT };
    for (int i = 0; i < 3; i++) {
        lbls[i] = lv_label_create(row);
        lv_label_set_text(lbls[i], "--:--");
        lv_obj_set_flex_grow(lbls[i], 1);
        lv_obj_set_height(lbls[i], LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(lbls[i], lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(lbls[i], &ui_font_ver14, 0);
        lv_obj_set_style_text_align(lbls[i], align[i], 0);
    }
}

static void set_time_lbls(lv_obj_t **lbls, time_t t0, time_t t1, time_t t2, bool show_day)
{
    static const char *wday[] = {"So","Mo","Di","Mi","Do","Fr","Sa"};
    time_t ts[3] = { t0, t1, t2 };
    for (int i = 0; i < 3; i++) {
        if (!lbls[i]) continue;
        struct tm tm; localtime_r(&ts[i], &tm);
        char buf[12];
        if (show_day)
            snprintf(buf, sizeof(buf), "%s %02d:%02d", wday[tm.tm_wday], tm.tm_hour, tm.tm_min);
        else
            snprintf(buf, sizeof(buf), "%02d:%02d", tm.tm_hour, tm.tm_min);
        lv_label_set_text(lbls[i], buf);
    }
}

// ─── Build Tab "Temp" ─────────────────────────────────────────────────────────
static void build_tab_temp(lv_obj_t *tab)
{
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(tab, LV_GRAD_DIR_NONE, 0);
    lv_obj_set_style_pad_all(tab, 0, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    const int tc_leg_h = 32;
    const int tc_row_h = 20;
    const int tc_h     = DISPLAY_HEIGHT - 40 - tc_row_h - tc_leg_h;

    s_tc = make_chart(tab, DISPLAY_WIDTH - 50, tc_h, 50, 0,
                      -100, 400, 4, 7);  // -10°C … +40°C (×10)
    lv_obj_set_style_pad_top(s_tc, 10, 0);
    lv_obj_set_style_pad_bottom(s_tc, 10, 0);
    s_tc_out = lv_chart_add_series(s_tc, lv_color_hex(0x4499FF), LV_CHART_AXIS_PRIMARY_Y);
    s_tc_liv = lv_chart_add_series(s_tc, lv_color_hex(0x44BB66), LV_CHART_AXIS_PRIMARY_Y);
    s_tc_lab = lv_chart_add_series(s_tc, lv_color_hex(0xFF9933), LV_CHART_AXIS_PRIMARY_Y);

    make_time_row(tab, tc_h, tc_row_h, s_tc_tlbl, 50);

    // Legend bar
    lv_obj_t *leg = lv_obj_create(tab);
    lv_obj_set_size(leg, DISPLAY_WIDTH, 32);
    lv_obj_align(leg, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(leg, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(leg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(leg, 0, 0);
    lv_obj_set_style_pad_hor(leg, 12, 0);
    lv_obj_set_style_pad_ver(leg, 4, 0);
    lv_obj_set_style_pad_gap(leg, 20, 0);
    lv_obj_clear_flag(leg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(leg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(leg, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(leg, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    struct { const char *txt; uint32_t col; } items[] = {
        {"Au\xc3\x9f""en", 0x4499FF}, {"Wohnzimmer", 0x44BB66}, {"Labor", 0xFF9933},
    };
    for (auto &it : items) {
        lv_obj_t *l = lv_label_create(leg);
        lv_label_set_text(l, it.txt);
        lv_obj_set_style_text_color(l, lv_color_hex(it.col), 0);
        lv_obj_set_style_text_font(l, &ui_font_ver14, 0);
    }
}

// ─── Build Tab "Energie" (power sensors only) ────────────────────────────────
static void build_tab_energie(lv_obj_t *tab)
{
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(tab, LV_GRAD_DIR_NONE, 0);
    lv_obj_set_style_pad_all(tab, 0, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    const int leg_h = 34;
    const int row_h = 20;
    const int pwr_h = DISPLAY_HEIGHT - 40 - row_h - leg_h;

    s_pc = make_chart(tab, DISPLAY_WIDTH - 56, pwr_h, 56, 0, 0, 1500, 4, 7);
    lv_obj_set_style_pad_top(s_pc, 10, 0);
    lv_obj_set_style_pad_bottom(s_pc, 10, 0);
    s_pc_tas = lv_chart_add_series(s_pc, lv_color_hex(0xFF4444), LV_CHART_AXIS_PRIMARY_Y);
    s_pc_out = lv_chart_add_series(s_pc, lv_color_hex(0xFFDD00), LV_CHART_AXIS_PRIMARY_Y);
    s_pc_s1  = lv_chart_add_series(s_pc, lv_color_hex(0x66AAFF), LV_CHART_AXIS_PRIMARY_Y);
    s_pc_s2  = lv_chart_add_series(s_pc, lv_color_hex(0x44DDCC), LV_CHART_AXIS_PRIMARY_Y);
    s_pc_s3  = lv_chart_add_series(s_pc, lv_color_hex(0xFF8844), LV_CHART_AXIS_PRIMARY_Y);
    s_pc_s4  = lv_chart_add_series(s_pc, lv_color_hex(0xAA66FF), LV_CHART_AXIS_PRIMARY_Y);

    make_time_row(tab, pwr_h, row_h, s_ec_tlbl, 56);

    lv_obj_t *leg = lv_obj_create(tab);
    lv_obj_set_size(leg, DISPLAY_WIDTH, leg_h);
    lv_obj_align(leg, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(leg, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(leg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(leg, 0, 0);
    lv_obj_set_style_pad_hor(leg, 8, 0);
    lv_obj_set_style_pad_ver(leg, 4, 0);
    lv_obj_set_style_pad_gap(leg, 10, 0);
    lv_obj_clear_flag(leg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(leg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(leg, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(leg, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    struct { const char *txt; uint32_t col; } items[] = {
        {"Haus",  0xFF4444},
        {"Solar", 0xFFDD00},
        {"P1",    0x66AAFF},
        {"P2",    0x44DDCC},
        {"P3",    0xFF8844},
        {"P4",    0xAA66FF},
    };
    for (auto &it : items) {
        lv_obj_t *l = lv_label_create(leg);
        lv_label_set_text(l, it.txt);
        lv_obj_set_style_text_color(l, lv_color_hex(it.col), 0);
        lv_obj_set_style_text_font(l, &ui_font_ver14, 0);
    }
}

// ─── Build Tab "Bat" (battery SoC) ───────────────────────────────────────────
static void build_tab_bat(lv_obj_t *tab)
{
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(tab, LV_GRAD_DIR_NONE, 0);
    lv_obj_set_style_pad_all(tab, 0, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    const int leg_h = 32;
    const int row_h = 20;
    const int bat_h = DISPLAY_HEIGHT - 40 - row_h - leg_h;

    s_bc = make_chart(tab, DISPLAY_WIDTH - 48, bat_h, 48, 0, 0, 100, 4, 7);
    lv_obj_set_style_pad_top(s_bc, 10, 0);
    lv_obj_set_style_pad_bottom(s_bc, 10, 0);
    s_bc_bat = lv_chart_add_series(s_bc, lv_color_hex(0x44DDAA), LV_CHART_AXIS_PRIMARY_Y);

    make_time_row(tab, bat_h, row_h, s_bc_tlbl, 48);

    lv_obj_t *leg = lv_obj_create(tab);
    lv_obj_set_size(leg, DISPLAY_WIDTH, leg_h);
    lv_obj_align(leg, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(leg, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(leg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(leg, 0, 0);
    lv_obj_set_style_pad_hor(leg, 12, 0);
    lv_obj_set_style_pad_ver(leg, 4, 0);
    lv_obj_set_style_pad_gap(leg, 20, 0);
    lv_obj_clear_flag(leg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(leg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(leg, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(leg, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *l = lv_label_create(leg);
    lv_label_set_text(l, "Batterie %");
    lv_obj_set_style_text_color(l, lv_color_hex(0x44DDAA), 0);
    lv_obj_set_style_text_font(l, &ui_font_ver14, 0);

}

static void stab0_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 0, LV_ANIM_OFF);
}
static void stab1_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 1, LV_ANIM_OFF);
}
static void stab2_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 2, LV_ANIM_OFF);
}
static void stab3_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 3, LV_ANIM_OFF);
}
static void stab4_cb(lv_event_t *e)
{
    lv_tabview_set_act((lv_obj_t *)lv_event_get_user_data(e), 4, LV_ANIM_OFF);
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

bool osiris_bottom_init(void)
{
    apply_style();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(scr, &style_header_footer, LV_PART_MAIN);

    s_tv = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(s_tv, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(s_tv, 50);
    lv_obj_set_pos(s_tv, 0, 0);
    lv_obj_set_size(s_tv, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_add_style(s_tv, &style_header_footer, LV_PART_MAIN);
    lv_obj_add_style(s_tv, &style_tabview, LV_PART_MAIN);

    lv_obj_t *tab_default   = lv_tabview_add_tab(s_tv, "Uhr");
    lv_obj_t *tab_steuerung = lv_tabview_add_tab(s_tv, "Steuerung");
    lv_obj_t *tab_temp      = lv_tabview_add_tab(s_tv, "Temp");
    lv_obj_t *tab_energie   = lv_tabview_add_tab(s_tv, "Energie");
    lv_obj_t *tab_bat       = lv_tabview_add_tab(s_tv, "Bat");

    lv_obj_add_style(tab_default,   &style_header_footer, LV_PART_MAIN);
    lv_obj_add_style(tab_steuerung, &style_header_footer, LV_PART_MAIN);
    lv_obj_add_style(tab_temp,      &style_header_footer, LV_PART_MAIN);
    lv_obj_add_style(tab_energie,   &style_header_footer, LV_PART_MAIN);
    lv_obj_add_style(tab_bat,       &style_header_footer, LV_PART_MAIN);

    build_tab_default(tab_default);
    build_tab_steuerung(tab_steuerung);
    build_tab_temp(tab_temp);
    build_tab_energie(tab_energie);
    build_tab_bat(tab_bat);

    // Hide native text tab buttons — replaced with icon bar below
    lv_obj_t *native_btns = lv_tabview_get_tab_btns(s_tv);
    lv_obj_add_flag(native_btns, LV_OBJ_FLAG_HIDDEN);

    // Floating icon bar in the space reserved by LV_DIR_BOTTOM, 50
    lv_obj_t *bar = lv_obj_create(s_tv);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, LV_PCT(100), 50);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(bar, &style_header_footer, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_clear_flag(lv_tabview_get_content(s_tv), LV_OBJ_FLAG_SCROLLABLE);

    make_inner_tab_btn(bar, &clock_icon,  stab0_cb, s_tv);
    make_inner_tab_btn(bar, &steering,    stab1_cb, s_tv);
    make_inner_tab_btn(bar, &temperature, stab2_cb, s_tv);
    make_inner_tab_btn(bar, &energy,      stab3_cb, s_tv);
    make_inner_tab_btn(bar, &battery,     stab4_cb, s_tv);
    
    fprintf(stderr, "[horus] screen built OK\n");
    return true;
}

void osiris_bottom_tick(void)
{
    
}
