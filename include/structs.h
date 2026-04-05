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

// 职称 

typedef enum {
    TITLE_RESIDENT   = 1,   // 住院医师   +0元
    TITLE_ATTENDING  = 2,   // 主治医师   +5元
    TITLE_ASSOCIATE  = 3,   // 副主任医师 +15元
    TITLE_CHIEF      = 4    // 主任医师   +30元
} DoctorTitle;
 
// 病房类型 

typedef enum {
    ROOM_A = 1,   // 普通多人间  80元/天
    ROOM_B = 2,   // 术后观察   200元/天
    ROOM_C = 3    // 急诊留观   120元/天
} RoomType;
 
// 处方/检查状态 

typedef enum {
    STATUS_PENDING_PAY  = 1,   // 待缴费
    STATUS_PENDING_DO   = 2,   // 已缴费待执行
    STATUS_DONE         = 3    // 已完成
} ItemStatus;
 
// 检查结果 

typedef enum {
    EXAM_RESULT_NORMAL  = 1,   // 正常，无需处理
    EXAM_RESULT_DRUG    = 2,   // 需要开药
    EXAM_RESULT_INPATIENT = 3  // 需要住院
} ExamResult;

// 员工节点（管理员 / 医生 / 药剂师） 

typedef struct StaffNode {
    int          id;             // 工号
    char         password[50];
    char         name[50];
    UserRole     role;
    DoctorTitle  title;          // 仅医生使用，其他角色填0
    int          dept_id;        // 所属科室ID，仅医生使用
    struct StaffNode *next;
} StaffNode;
 
//患者节点 

typedef struct PatientNode {
    int  medical_id;       //病历号，系统自动生成，从 20001 起 
    char id_card[19];      //身份证号，仅用于登录验证，医护人员不可见 
    char password[50];
    char name[50];
    int  gender;           // 1=男 2=女
    int  age;
    struct PatientNode *next;
} PatientNode;
 
// 科室节点 
typedef struct DepartmentNode {
    int   dept_id;
    char  dept_name[50];
    float base_reg_fee;          // 挂号基础费用
    struct DepartmentNode *next;
} DepartmentNode;
 
// 检查项目节点 
typedef struct ExamItemNode {
    int   exam_id;
    int   dept_id;               // 所属科室
    char  exam_name[50];
    float price;
    struct ExamItemNode *next;
} ExamItemNode;
 
//  药品节点 
typedef struct DrugNode {
    int   drug_id;
    int   dept_id;               // 关联科室，0表示通用
    char  drug_name[50];
    float price;                 // 每盒单价
    int   stock;                 // 库存数量
    int   warning_line;          // 库存预警阈值
    struct DrugNode *next;
} DrugNode;
 
//  挂号记录节点 
typedef struct RegistrationNode {
    int    reg_id;
    int    patient_id;           // 病历号
    int    doctor_id;            // 医生工号
    int    dept_id;
    char   date[25];
    float  reg_fee;              // 实际挂号费
    int    queue_num;            // 排队号
    ItemStatus status;
    struct RegistrationNode *next;
} RegistrationNode;
 
//  病历节点 
typedef struct MedicalRecordNode {
    int    record_id;
    int    reg_id;               // 关联挂号记录
    int    patient_id;
    int    doctor_id;
    int    dept_id;
    char   diagnosis[200];       // 诊断内容
    char   date[25];
    struct MedicalRecordNode *next;
} MedicalRecordNode;

//  检查申请节点 
typedef struct ExamOrderNode {
    int        order_id;
    int        record_id;        // 关联病历
    int        patient_id;
    int        exam_id;          // 检查项目
    float      price;
    ItemStatus status;
    ExamResult result;           // 检查结果，STATUS_DONE后填写
    struct ExamOrderNode *next;
} ExamOrderNode;
 
//  处方节点 
typedef struct PrescriptionNode {
    int        prescription_id;
    int        record_id;        // 关联病历
    int        patient_id;
    int        drug_id;
    int        quantity;         // 数量（盒）
    float      price;            // 总价
    ItemStatus status;
    struct PrescriptionNode *next;
} PrescriptionNode;
 
//  病房节点 
typedef struct RoomNode {
    int      room_id;
    int      dept_id;            // 所属科室
    RoomType room_type;
    int      capacity;           // 总床位数
    int      current;            // 当前住院人数
    float    daily_fee;          // 每日费用
    struct RoomNode *next;
} RoomNode;
 
//  住院记录节点 
typedef struct InpatientNode {
    int   inpatient_id;          // 住院人ID
    int   patient_id;
    int   doctor_id;
    int   dept_id;
    int   room_id;
    int   bed_num;               // 床位编号
    char  admit_date[25];        // 入院日期
    char  discharge_date[25];    // 出院日期，未出院填 "-"
    int   days;                  // 住院天数，出院时计算
    float total_fee;             // 住院总费用
    int   is_discharged;         // 0=在院 1=已出院
    struct InpatientNode *next;
} InpatientNode;
 
//当前登录用户（全局唯一）

typedef struct {
    int      is_logged_in; //1=已登录，0=未登录 
    int      user_id;      //员工→工号；患者→病历号 
    char     user_name[50];
    UserRole user_role;
    int      dept_id;            // 仅医生使用
    DoctorTitle title;           // 仅医生使用
} CurrentUser;
 
#endif
