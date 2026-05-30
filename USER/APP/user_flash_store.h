/**
 * @file user_flash_store.h
 * @brief Persistent settings storage on W25Q64.
 */

#ifndef UF4DIGITALPOWER_USER_FLASH_STORE_H
#define UF4DIGITALPOWER_USER_FLASH_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void USER_flashStoreInitApp(void);
void USER_flashStoreRequestAppSave(void);
void USER_flashStoreRunAppTask(void);
uint8_t USER_flashStoreIsAppReady(void);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_USER_FLASH_STORE_H */
