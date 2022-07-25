#include "configuration.h"
#include "globals.h"

void ConfigureOffsetValuesVID(USHORT usControlFlag)

{
    if (usControlFlag == 1)
    {
        ALT_RATING1_OFFSET = 0;
        PA_ID_OFFSET = 1;
        ALT_RATING2_OFFSET = 2;
        COOLING_FAN_OFFSET = 4;
        SNOW_PLOW_OFFSET = 5;
        AGS_OFFSET = 5;
        AAT_OFFSET = 6;
        EOC_OFFSET = 6;
        AEIS_OFFSET = 7;
        EOTC_OFFSET = 8;
        MEFC_OFFSET = 8;
        VIN_OFFSET = 0x40;
        ASTERISK_OFFSET = 0x55;
        OCTANE_ADJ_OFFSET = 0x56;
        OCT_RATING_OFFSET = 0x57;
        SPEED_LIMIT_OFFSET = 0x58;
        TIRE_OFFSET = 0x59;
        AXLE_OFFSET = 0x5a;
        LOW_RANGE_OFFSET = 0x5b;
        TCASE_OFFSET = 0x5c;
        SPEED_CTL_OFFSET = 0x5d;
        SPEED_SRCE_OFFSET = 0x5e;
        SC_TYPE_OFFSET = 0x5f;
        DRIVETRAIN_OFFSET = 0x60;
        WHEEL_BASE_OFFSET = 0x61;
        SHUTDOWN_OFFSET = 0x62;
        KEYLESS_OFFSET = 99;
        SST_OFFSET = 100;
        DRIVE_AXLE_OFFSET = 0x65;
        ASL_OFFSET = 0x66;
        TMM_OFFSET = 0x67;
        TOW_HAUL_OFFSET = 0x68;
        RING_GEAR_OFFSET = 0x69;
        OPTIONAL_ECU_OFFSET1 = 0x6a;
        OPTIONAL_ECU_OFFSET2 = 0x6b;
    }
    else if (usControlFlag == 2)
    {
        DRIVETRAIN_OFFSET = 0;
        SPEED_SRCE_OFFSET = 1;
        SPEED_LIMIT_OFFSET = 2;
        RING_GEAR_OFFSET = 3;
        TOW_HAUL_OFFSET = 4;
        SST_OFFSET = 4;
        DRIVE_AXLE_OFFSET = 5;
        TMM_OFFSET = 5;
        ASL_OFFSET = 6;
        KEYLESS_OFFSET = 6;
        WHEEL_BASE_OFFSET = 7;
        OPTIONAL_ECU_OFFSET1 = 8;
        OPTIONAL_ECU_OFFSET2 = 9;
        VIN_OFFSET = 0x40;
        ASTERISK_OFFSET = 0x55;
        OCTANE_ADJ_OFFSET = 0x56;
        OCT_RATING_OFFSET = 0x57;
        TIRE_OFFSET = 0x58;
        AXLE_OFFSET = 0x59;
        LOW_RANGE_OFFSET = 0x5a;
        TCASE_OFFSET = 0x5b;
        SPEED_CTL_OFFSET = 0x5c;
        SC_TYPE_OFFSET = 0x5d;
        SHUTDOWN_OFFSET = 0x5e;
        ALT_RATING1_OFFSET = 0x5f;
        PA_ID_OFFSET = 0x60;
        ALT_RATING2_OFFSET = 0x61;
        COOLING_FAN_OFFSET = 0x62;
        SNOW_PLOW_OFFSET = 99;
        AGS_OFFSET = 100;
        AAT_OFFSET = 0x65;
        EOC_OFFSET = 0x66;
        AEIS_OFFSET = 0x67;
        EOTC_OFFSET = 0x68;
        MEFC_OFFSET = 0x69;
    }
    else if (usControlFlag == 0)
    {
        VIN_OFFSET = 0;
        ASTERISK_OFFSET = 0x15;
        OCTANE_ADJ_OFFSET = 0x16;
        OCT_RATING_OFFSET = 0x17;
        SPEED_LIMIT_OFFSET = 0x19;
        TIRE_OFFSET = 0x1a;
        AXLE_OFFSET = 0x1c;
        LOW_RANGE_OFFSET = 0x1f;
        TCASE_OFFSET = 0x1f;
        SPEED_CTL_OFFSET = 0x1f;
        ALT_RATING1_OFFSET = 0x20;
        SPEED_SRCE_OFFSET = 0x21;
        EOTC_OFFSET = 0x22;
        SC_TYPE_OFFSET = 0x22;
        ALT_RATING2_OFFSET = 0x23;
        DRIVETRAIN_OFFSET = 0x24;
        WHEEL_BASE_OFFSET = 0x25;
        SNOW_PLOW_OFFSET = 0x26;
        SHUTDOWN_OFFSET = 0x26;
        KEYLESS_OFFSET = 0x27;
        PA_ID_OFFSET = 0x28;
        AAT_OFFSET = 0x29;
        AGS_OFFSET = 0x29;
        SST_OFFSET = 0x2a;
        EOC_OFFSET = 0x2b;
        MEFC_OFFSET = 0x2c;
        DRIVE_AXLE_OFFSET = 0x2d;
        ASL_OFFSET = 0x2e;
        TMM_OFFSET = 0x2f;
        TOW_HAUL_OFFSET = 0x2f;
        RING_GEAR_OFFSET = 0x30;
        COOLING_FAN_OFFSET = 0x31;
        OPTIONAL_ECU_OFFSET1 = 0x66;
        OPTIONAL_ECU_OFFSET2 = 0x67;
        AEIS_OFFSET = 0x68;
    }
}
