#ifndef STRUCTS_H
#define STRUCTS_H

 
#define MAX_PATIENTS 200
#define MAX_STAFF    80

// 用户类型枚举

typedef enum {
    ROLE_ADMIN = 1,        // 管理员
    ROLE_DOCTOR = 2,       // 医生
    ROLE_PHARMACIST = 3,   // 药剂师
    ROLE_PATIENT = 4       // 患者
} UserRole;

// 员工节点（管理员 / 医生 / 药剂师） 

typedef struct StaffNode {
    int  id;               
    char password[50];
    char name[50];
    UserRole role;
    struct StaffNode *next;
} StaffNode;
 
//患者节点 

typedef struct PatientNode {
    int  medical_id;       //病历号，系统自动生成，从 20001 起 
    char id_card[19];      //身份证号，仅用于登录验证，医护人员不可见 
    char password[50];
    char name[50];
    struct PatientNode *next;
} PatientNode;
 
//当前登录用户（全局唯一）

typedef struct {
    int      is_logged_in; //1=已登录，0=未登录 
    int      user_id;      //员工→工号；患者→病历号 
    char     user_name[50];
    UserRole user_role;
} CurrentUser;
 
#endif
