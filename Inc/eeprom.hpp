#pragma once

#include <array>
#include <bit>
#include <expected>
#include <limits>
#include <numeric>
#include <type_traits>

#ifdef __cplusplus
extern "C" {
#endif

#include "eeprom_config.hpp"
#include "eeprom_emul.h"
#include "eeprom_emul_types.h"

#ifdef __cplusplus
}
#endif

/** @brief Ensure type is proper size for X-CUBE-EEPROM, and can be bit_casted
 */
template <typename TYPE>
concept EepromVariableSize =
    std::is_trivially_constructible_v<TYPE> &&
    std::is_trivially_copyable_v<TYPE> &&
    (sizeof(TYPE) == 1 || sizeof(TYPE) == 2 || sizeof(TYPE) == 4);

/** @brief Ensure user create own enum class that can be casted to uint16_t
 */
template <typename TYPE>
concept EepromAddressType =
    std::is_enum_v<TYPE> &&
    std::same_as<std::underlying_type_t<TYPE>, uint16_t>;

/** @brief Eeprom class holds state of eeprom initialization and ensures that no
 * more than maximum variables count(defined in eeprom_emul_conf.h) can be
 * created
 */
template <EepromAddressType ADDRESS_TYPE> class Eeprom {
  private:
    static constexpr uint32_t MAX_VARIABLE_COUNT = NB_OF_VARIABLES;
    uint32_t variable_count = 0;
    bool is_initialized = false;
    bool cleanup_required = false;
    std::array<bool, MAX_VARIABLE_COUNT> address_map{};

  public:
    Eeprom() = default;

    enum class VariableState : uint8_t {
        NOT_INITIALIZED,
        INITIALIZED,
        VALID_DATA,
        ERROR_DATA,
    };

    /** @brief Variable class used to initialize or read already initialized
     * data under @param new_address in eeprom.
     *  @param new_address is incremented by 1 because X-CUBE-EEPROM doesn't
     * allow for 0 address.
     * Adresses should be of ADDRESS_TYPE enum class and not be higher than
     * MAX_VARIABLE_COUNT-1
     */
    template <EepromVariableSize SIZE_TYPE> class Variable {
      private:
        Eeprom& eeprom;
        const uint16_t address;
        VariableState state = VariableState::NOT_INITIALIZED;

      public:
        Variable(Eeprom& new_eeprom, ADDRESS_TYPE new_address,
                 SIZE_TYPE init_value)
            : eeprom{new_eeprom},
              address{static_cast<uint16_t>(new_address) + 1} {

            // raw_new_address is passed directly by user(not incremented by 1)
            // address is used by X-CUBE-EEPROM(incremented by 1)
            uint16_t raw_new_address = static_cast<uint16_t>(new_address);

            if(!eeprom.is_initialized ||
               eeprom.variable_count >= Eeprom::MAX_VARIABLE_COUNT ||
               raw_new_address >= Eeprom::MAX_VARIABLE_COUNT ||
               raw_new_address == std::numeric_limits<uint16_t>::max() ||
               eeprom.address_map[raw_new_address] == true) {
                return;
            }

            state = VariableState::INITIALIZED;
            eeprom.address_map[raw_new_address] = true;
            ++eeprom.variable_count;

            if(!read()) {
                write(init_value);
            }
        }

        [[nodiscard]] std::expected<SIZE_TYPE, HAL_StatusTypeDef> read() {

            if(!eeprom.is_initialized ||
               state == VariableState::NOT_INITIALIZED)
                return std::unexpected(HAL_ERROR);

            SIZE_TYPE read_value{};

            EE_Status status = EE_OK;
            if constexpr(sizeof(SIZE_TYPE) == 1) {
                uint8_t buffer{};
                status = EE_ReadVariable8bits(address, &buffer);
                read_value = std::bit_cast<SIZE_TYPE>(buffer);
            } else if constexpr(sizeof(SIZE_TYPE) == 2) {
                uint16_t buffer{};
                status = EE_ReadVariable16bits(address, &buffer);
                read_value = std::bit_cast<SIZE_TYPE>(buffer);
            } else if constexpr(sizeof(SIZE_TYPE) == 4) {
                uint32_t buffer{};
                status = EE_ReadVariable32bits(address, &buffer);
                read_value = std::bit_cast<SIZE_TYPE>(buffer);
            }
            if(status != EE_OK) {
                state = VariableState::ERROR_DATA;
                return std::unexpected(HAL_ERROR);
            }
            state = VariableState::VALID_DATA;
            return read_value;
        }

        HAL_StatusTypeDef write(SIZE_TYPE new_value) {

            if(!eeprom.is_initialized || eeprom.cleanup_required ||
               state == VariableState::NOT_INITIALIZED)
                return HAL_ERROR;

            EE_Status status = EE_OK;

            if constexpr(sizeof(SIZE_TYPE) == 1) {
                status = EE_WriteVariable8bits(
                    address, std::bit_cast<uint8_t>(new_value));
            } else if constexpr(sizeof(SIZE_TYPE) == 2) {
                status = EE_WriteVariable16bits(
                    address, std::bit_cast<uint16_t>(new_value));
            } else if constexpr(sizeof(SIZE_TYPE) == 4) {
                status = EE_WriteVariable32bits(
                    address, std::bit_cast<uint32_t>(new_value));
            }

            if(status == EE_CLEANUP_REQUIRED) {
                eeprom.cleanup_required = true;
                status = EE_OK;
            }

            if(status != EE_OK) {
                state = VariableState::ERROR_DATA;
                return HAL_ERROR;
            }

            state = VariableState::VALID_DATA;
            return HAL_OK;
        }

        HAL_StatusTypeDef erase() {
            if(!eeprom.is_initialized ||
               state == VariableState::NOT_INITIALIZED)
                return HAL_ERROR;

            EE_Status status = EE_OK;

            if constexpr(sizeof(SIZE_TYPE) == 1) {
                status = EE_WriteVariable8bits(address, 0xFF);
            } else if constexpr(sizeof(SIZE_TYPE) == 2) {
                status = EE_WriteVariable16bits(address, 0xFFFF);
            } else if constexpr(sizeof(SIZE_TYPE) == 4) {
                status = EE_WriteVariable32bits(address, 0xFFFFFFFF);
            }

            if(status == EE_CLEANUP_REQUIRED) {
                eeprom.cleanup_required = true;
                status = EE_OK;
            }

            if(status != EE_OK) {
                state = VariableState::ERROR_DATA;
                return HAL_ERROR;
            }

            // Get raw address
            eeprom.address_map[address - 1] = false;
            --eeprom.variable_count;
            state = VariableState::NOT_INITIALIZED;
            return HAL_OK;
        }

        [[nodiscard]] VariableState get_state() const { return state; }
    };

    HAL_StatusTypeDef init() {

        EE_Status status = EE_OK;
#ifdef EDATA_ENABLED
        mpu_config_edata();
#endif
        status = EE_Init(EE_CONDITIONAL_ERASE);
        if(status) {
            is_initialized = false;
            return HAL_ERROR;
        }

        is_initialized = true;
        return HAL_OK;
    }

    HAL_StatusTypeDef format() {
        EE_Status status = EE_OK;
        status = EE_Format(EE_FORCED_ERASE);
        if(status) {
            is_initialized = false;
            return HAL_ERROR;
        }

        variable_count = 0;
        address_map.fill(false);
        is_initialized = true;
        return HAL_OK;
    }

    HAL_StatusTypeDef cleanup() {
        if(!cleanup_required)
            return HAL_ERROR;
        EE_Status status = EE_OK;
        status = EE_CleanUp();
        if(status) {
            return HAL_ERROR;
        }
        cleanup_required = false;
        return HAL_OK;
    }

    [[nodiscard]] bool get_cleanup_required() const { return cleanup_required; }

    /** @brief Create new variable with init value
     *
     */
    template <EepromVariableSize SIZE_TYPE>
    Variable<SIZE_TYPE> new_var(ADDRESS_TYPE address, SIZE_TYPE init_value) {
        return Variable<SIZE_TYPE>{*this, address, init_value};
    }

    /** @brief Create new variable with expected read value at @param address
     *
     */
    template <EepromVariableSize SIZE_TYPE>
    Variable<SIZE_TYPE> new_var(ADDRESS_TYPE address) {
        return Variable<SIZE_TYPE>{*this, address, SIZE_TYPE{}};
    }
};

