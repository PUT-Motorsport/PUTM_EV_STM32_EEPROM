#pragma once

/* --- Configuration of eeprom emulation in flash, can be custom --- */

/* Start address of the 1st page in flash, for EEPROM emulation */
#define START_PAGE_ADDRESS 0x0900C000U

/* Number of 10Kcycles requested, minimum 1 for 10Kcycles (default), for
 * instance 10 to reach 100Kcycles. This factor will increase pages number */
#define CYCLES_NUMBER 1U

/* Number of guard pages avoiding frequent transfers (must be multiple of 2):
 * 0,2,4.. */
#define GUARD_PAGES_NUMBER 2U

/* Number of variables to handle in eeprom */
#define NB_OF_VARIABLES 10U

/* --- Configuration of crc calculation for eeprom emulation in flash --- */

/* CRC polynomial lenght 16 bits */
#define CRC_POLYNOMIAL_LENGTH LL_CRC_POLYLENGTH_16B

/* Polynomial to use for CRC calculation */
#define CRC_POLYNOMIAL_VALUE 0x8005U

#define EDATA_ENABLED true
