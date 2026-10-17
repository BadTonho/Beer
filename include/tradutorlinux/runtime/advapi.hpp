#pragma once

#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#define TL_ADVAPI_MSABI __attribute__((ms_abi))
#else
#error "TL_ADVAPI_MSABI requer GCC ou Clang em x86-64"
#endif

namespace tradutorlinux {

extern "C" {

TL_ADVAPI_MSABI std::uint32_t tl_RegCloseKey(const void* key) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegDeleteValueA(const void* key, const char* value_name) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegDeleteValueW(const void* key,
                                                 const std::uint16_t* value_name) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegCreateKeyExA(const void* key, const char* subkey,
                                                  std::uint32_t reserved, char* class_name,
                                                  std::uint32_t options, std::uint32_t access,
                                                  const void* security_attributes, void** result,
                                                  std::uint32_t* disposition) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegCreateKeyExW(const void* key,
                                                  const std::uint16_t* subkey,
                                                  std::uint32_t reserved, std::uint16_t* class_name,
                                                  std::uint32_t options, std::uint32_t access,
                                                  const void* security_attributes, void** result,
                                                  std::uint32_t* disposition) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegOpenKeyExA(const void* key, const char* subkey,
                                                std::uint32_t options, std::uint32_t access,
                                                void** result) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegOpenKeyExW(const void* key, const std::uint16_t* subkey,
                                                std::uint32_t options, std::uint32_t access,
                                                void** result) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegQueryValueExA(const void* key, const char* value_name,
                                                   std::uint32_t* reserved, std::uint32_t* type,
                                                   unsigned char* data,
                                                   std::uint32_t* data_size) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegQueryValueExW(const void* key,
                                                   const std::uint16_t* value_name,
                                                   std::uint32_t* reserved, std::uint32_t* type,
                                                   unsigned char* data,
                                                   std::uint32_t* data_size) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegSetValueExA(const void* key, const char* value_name,
                                                 std::uint32_t reserved, std::uint32_t type,
                                                 const unsigned char* data,
                                                 std::uint32_t data_size) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegSetValueExW(const void* key,
                                                 const std::uint16_t* value_name,
                                                 std::uint32_t reserved, std::uint32_t type,
                                                 const unsigned char* data,
                                                 std::uint32_t data_size) noexcept;

TL_ADVAPI_MSABI int tl_CryptAcquireContextA(void** prov_handle, const char* container,
                                            const char* provider, std::uint32_t prov_type,
                                            std::uint32_t flags) noexcept;
TL_ADVAPI_MSABI int tl_CryptAcquireContextW(void** prov_handle, const std::uint16_t* container,
                                            const std::uint16_t* provider, std::uint32_t prov_type,
                                            std::uint32_t flags) noexcept;
TL_ADVAPI_MSABI int tl_CryptGenRandom(void* prov_handle, std::uint32_t length,
                                      std::uint8_t* buffer) noexcept;
TL_ADVAPI_MSABI int tl_CryptReleaseContext(void* prov_handle, std::uint32_t flags) noexcept;
TL_ADVAPI_MSABI int tl_CryptImportKey(std::uintptr_t prov, const std::uint8_t* data,
                                      std::uint32_t data_len, std::uintptr_t pub_key,
                                      std::uint32_t flags, std::uintptr_t* key) noexcept;
TL_ADVAPI_MSABI int tl_CryptEncrypt(std::uintptr_t key, std::uintptr_t hash,
                                    int final_chunk, std::uint32_t flags,
                                    std::uint8_t* data, std::uint32_t* data_len,
                                    std::uint32_t buf_len) noexcept;

TL_ADVAPI_MSABI int tl_OpenProcessToken(const void* process, std::uint32_t desired_access,
                                        void** token) noexcept;
TL_ADVAPI_MSABI int tl_GetTokenInformation(const void* token, std::uint32_t information_class,
                                           void* information, std::uint32_t information_length,
                                           std::uint32_t* return_length) noexcept;
TL_ADVAPI_MSABI int tl_AllocateAndInitializeSid(const void* identifier_authority,
                                                std::uint8_t sub_authority_count,
                                                std::uint32_t sub_authority0,
                                                std::uint32_t sub_authority1,
                                                std::uint32_t sub_authority2,
                                                std::uint32_t sub_authority3,
                                                std::uint32_t sub_authority4,
                                                std::uint32_t sub_authority5,
                                                std::uint32_t sub_authority6,
                                                std::uint32_t sub_authority7,
                                                void** sid) noexcept;
TL_ADVAPI_MSABI void* tl_FreeSid(void* sid) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_GetLengthSid(const void* sid) noexcept;
TL_ADVAPI_MSABI int tl_CopySid(std::uint32_t destination_length, void* destination,
                               const void* source) noexcept;
TL_ADVAPI_MSABI int tl_EqualSid(const void* first, const void* second) noexcept;
TL_ADVAPI_MSABI int tl_IsValidSid(const void* sid) noexcept;
TL_ADVAPI_MSABI int tl_CreateWellKnownSid(std::uint32_t well_known_sid_type,
                                          const void* domain_sid, void* sid,
                                          std::uint32_t* sid_size) noexcept;
TL_ADVAPI_MSABI int tl_CheckTokenMembership(const void* token, const void* sid,
                                            int* is_member) noexcept;
TL_ADVAPI_MSABI void tl_BuildTrusteeWithSidW(void* trustee, void* sid) noexcept;
TL_ADVAPI_MSABI int tl_InitializeAcl(void* acl, std::uint32_t acl_length,
                                     std::uint32_t acl_revision) noexcept;
TL_ADVAPI_MSABI int tl_InitializeSecurityDescriptor(void* descriptor,
                                                     std::uint32_t revision) noexcept;
TL_ADVAPI_MSABI int tl_SetSecurityDescriptorDacl(void* descriptor, int dacl_present,
                                                 void* dacl, int dacl_defaulted) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_SetEntriesInAclW(std::uint32_t entry_count,
                                                  const void* entries, const void* old_acl,
                                                  void** new_acl) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_GetNamedSecurityInfoW(const std::uint16_t* object_name,
                                                       std::uint32_t object_type,
                                                       std::uint32_t security_information,
                                                       void** owner, void** group, void** dacl,
                                                       void** sacl, void** descriptor) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_SetNamedSecurityInfoW(std::uint16_t* object_name,
                                                       std::uint32_t object_type,
                                                       std::uint32_t security_information,
                                                       void* owner, void* group, void* dacl,
                                                       void* sacl) noexcept;
TL_ADVAPI_MSABI int tl_SetFileSecurityW(const std::uint16_t* file_name,
                                        std::uint32_t security_information,
                                        const void* security_descriptor) noexcept;
TL_ADVAPI_MSABI int tl_ConvertSidToStringSidW(const void* sid, std::uint16_t** string_sid) noexcept;
TL_ADVAPI_MSABI int tl_ConvertStringSidToSidW(const std::uint16_t* string_sid, void** sid) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_GetSecurityDescriptorLength(const void* security_descriptor) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_EventRegister(const void* provider_id, void* enable_callback,
                                               void* callback_context, std::uint64_t* reg_handle) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_EventUnregister(std::uint64_t reg_handle) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_EventSetInformation(std::uint64_t reg_handle, int information_class,
                                                    void* event_information, std::uint32_t information_length) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_EventWriteTransfer(std::uint64_t reg_handle, const void* event_descriptor,
                                                   const void* activity_id, const void* related_activity_id,
                                                   std::uint32_t user_data_count, void* user_data) noexcept;
TL_ADVAPI_MSABI int tl_AccessCheck(const void* sec_desc, void* token, std::uint32_t desired_access,
                                   const void* generic_mapping, void* priv_set, std::uint32_t* priv_set_len,
                                   std::uint32_t* granted_access, int* access_status) noexcept;
TL_ADVAPI_MSABI int tl_DuplicateToken(void* existing_token, int impersonation_level, void** duplicate_token) noexcept;
TL_ADVAPI_MSABI void tl_MapGenericMask(std::uint32_t* access_mask, const void* generic_mapping) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegOpenKeyA(const void* key, const char* subkey, void** result) noexcept;
TL_ADVAPI_MSABI std::uint32_t tl_RegQueryValueW(const void* key, const std::uint16_t* subkey, std::uint16_t* data, std::int32_t* data_size) noexcept;
TL_ADVAPI_MSABI std::int32_t tl_RegEnumKeyW(const void* key, std::uint32_t index, std::uint16_t* name, std::uint32_t name_size) noexcept;

TL_ADVAPI_MSABI void* tl_OpenSCManagerW(const std::uint16_t* machine_name, const std::uint16_t* database_name, std::uint32_t desired_access) noexcept;
TL_ADVAPI_MSABI int tl_CloseServiceHandle(void* handle) noexcept;
TL_ADVAPI_MSABI void* tl_OpenServiceW(void* sc_manager, const std::uint16_t* service_name, std::uint32_t desired_access) noexcept;
TL_ADVAPI_MSABI void* tl_CreateServiceW(void* sc_manager, const std::uint16_t* service_name, const std::uint16_t* display_name,
                                        std::uint32_t desired_access, std::uint32_t service_type, std::uint32_t start_type,
                                        std::uint32_t error_control, const std::uint16_t* binary_path,
                                        const std::uint16_t* load_order_group, std::uint32_t* tag_id,
                                        const std::uint16_t* dependencies, const std::uint16_t* service_start_name,
                                        const std::uint16_t* password) noexcept;
TL_ADVAPI_MSABI int tl_DeleteService(void* service) noexcept;
TL_ADVAPI_MSABI int tl_StartServiceW(void* service, std::uint32_t num_service_args, const std::uint16_t** service_arg_vectors) noexcept;
TL_ADVAPI_MSABI int tl_ControlService(void* service, std::uint32_t control, void* service_status) noexcept;
TL_ADVAPI_MSABI int tl_SetServiceStatus(void* status_handle, const void* service_status) noexcept;
TL_ADVAPI_MSABI int tl_QueryServiceStatusEx(void* service, std::uint32_t info_level, unsigned char* buffer, std::uint32_t buf_size, std::uint32_t* bytes_needed) noexcept;
TL_ADVAPI_MSABI int tl_QueryServiceConfigW(void* service, void* service_config, std::uint32_t buf_size, std::uint32_t* bytes_needed) noexcept;
TL_ADVAPI_MSABI void* tl_RegisterServiceCtrlHandlerW(const std::uint16_t* service_name, void* handler_proc) noexcept;
TL_ADVAPI_MSABI int tl_StartServiceCtrlDispatcherW(const void* service_table) noexcept;

}  // extern "C"

}  // namespace tradutorlinux
