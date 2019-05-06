#ifndef HID_DEF_H
#define HID_DEF_H
/////////////////////////////////////////////

// These definitions are designed to permit compiling the HID report descriptors
// with somewhat self-explanatory information to help readability and reduce errors

//input,output,feature flags
#define Data_Arr_Abs                            0x00u
#define Const_Arr_Abs                           0x01u
#define Data_Var_Abs                            0x02u
#define Const_Var_Abs                           0x03u
#define Data_Var_Rel                            0x06u
//collection flags
#define Physical                                0x00u
#define Application                             0x01u
#define Logical                                 0x02u
#define NamedArray                              0x04u
#define UsageSwitch                             0x05u
//other
#define Undefined                               0x00u

#define HID_USAGE_PAGE(a)                       0x05u,a
#define HID_USAGE_PAGE16(a, b)                  0x06u,a,b
#define HID_USAGE(a)                            0x09u,a
#define HID_USAGE16(a,b)                        0x0Au,a,b
#define HID_USAGE_SENSOR_DATA(a,b)              a|b     //This or-s the mod into usage
#define HID_COLLECTION(a)                       0xA1u,a
#define HID_REPORT_ID(a)                        0x85u,a
#define HID_REPORT_SIZE(a)                      0x75u,a
#define HID_REPORT_COUNT(a)                     0x95u,a
#define HID_USAGE_MIN_8(a)                      0x19u,a
#define HID_USAGE_MIN_16(a,b)                   0x1Au,a,b
#define HID_USAGE_MAX_8(a)                      0x29u,a
#define HID_USAGE_MAX_16(a,b)                   0x2Au,a,b
#define HID_LOGICAL_MIN_8(a)                    0x15u,a
#define HID_LOGICAL_MIN_16(a,b)                 0x16u,a,b
#define HID_LOGICAL_MIN_32(a,b,c,d)             0x17u,a,b,c,d
#define HID_LOGICAL_MAX_8(a)                    0x25u,a
#define HID_LOGICAL_MAX_16(a,b)                 0x26u,a,b
#define HID_LOGICAL_MAX_32(a,b,c,d)             0x27u,a,b,c,d
#define HID_UNIT_EXPONENT(a)                    0x55u,a
#define HID_INPUT(a)                            0x81u,a
#define HID_OUTPUT(a)                           0x91u,a
#define HID_FEATURE(a)                          0xB1u,a
#define HID_END_COLLECTION()                    0xC0u

#define GenericDesktop                          0x01u
#define UsageMouse                              0x02u


#endif // HID_DEF_H

