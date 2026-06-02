A. 命名规范

1. 文件名：
    全部小写，单词之间用下划线分隔。例如：file_algo.cpp。

2. 类名、函数名：
    单词首字母大写，其余小写，没有下划线。例如：class FileHandler，GetFilePath()。

3. 类成员变量：
    1. 变量名前加下划线 _ 为变量是否 private 的标志，有下划线表示 private, protected, 否则为 public 
    2. 第一个单词全小写，后面单词首字母大写, 其余小写, 中间不带任何下划线, 驼峰式, 例如 "std::string _strFilePath" 
    3. 禁止在类成员声明时初始化, 成员必须在构造函数初始化列表中进行初始化（纯数据结构体除外），即禁止以下写法：
    Struct A {
        public: A(): _b(0){}    // 允许的写法, 但更推荐放在 CPP 中
        public: 
            int a;  // 规范的写法
            int _c; // 允许但不推荐的写法
        private:
            int _b = 0; // 禁止的写法
            int d;  // 禁止的写法
            int _f; // 规范的写法
    };

4. 函数局部变量、函数形参：
    全部小写，单词之间用下划线分割，开头一般不带下划线。例如：std::string file_path。

5. 全局变量：
    加 g_ 前缀，后面按照类成员变量规则。例如 "g_filePtr"

6. 静态变量：
    加 s_ 前缀，后面按照类成员变量规则。例如 "s_filePath"

7. 宏定义：
    全部大写，单词之间用下划线分隔。例如 "ERROR_VALUE"

8. 枚举：
    1. 命名：枚举名称按照类名规则，枚举值按宏定义规则。例如 enum ErrorType {...}
    2. 编码：
        enum ErrorType {...} // 不推荐
        enum ErrorType: int {...}   // 推荐

B. 代码规范

1. auto 的使用：
    非必要禁止使用 auto（迭代器和 lambda 表达式除外）。

2. 三目运算符：
    保证：两边的类型相同。
    常见错误1：
        表达式 ? 调用无返回值的函数 : 0;
    请修改成
        表达式 ? 调用无返回值的函数 : void();
    常见错误2：
        gp_Dir a = gp::DZ() : b;    // b 的类型为 gp_Vec
    应该更正为
        gp_Dir a = gp::DZ() : gp_Dir(b);

3. 关于接口形参
    1. const
        1. 函数形参【优先】以常引用传递，加 const
        2. 成员函数【优先】限定为常函数，加 const
            void Func(MyData md);                         // 不推荐
            void Func(MyData& md);                        // 非必要不推荐，记为 <1.2.2>
            void Func(const MyData& md);                  // 推荐
            void Func(const MyData& md) const;            // 推荐，记为 <1.2.4>
    2. 智能指针
        非必要不使用
            void Func(std::shared_ptr<MyData>& ptr);              // 非必要不推荐，推荐用 <1.2.2> 代替
            void Func(const std::shared_ptr<MyData>& ptr);        // 非必要不推荐，推荐用 <1.2.4> 代替
        原因：
            1. 智能指针作形参，如果用户手头没有智能指针，将会很麻烦
            2. 变量引用版本适用于更多情况
            3. 除非 A 想要将智能指针传给 B，让 B 也持有
    3. 输入、输出
        输出值，可以引用形式，以指针形式，推荐放末尾
        避免同一个形参既是输入值，又是输出值
            void Func(int& result, const MyData& md);                       // (a) 禁止：输出值不在末尾
            void Func(const MyData& md, int& remained);                     // (b) 禁止：remained 传入数量，并返回剩余数量
            void Func(const MyData& md, int& result, bool show = false);    // (c) 允许，但不推荐：因默认参数必须在末尾
            void FuncShow(const MyData& md, int& result);                   // (d) 推荐使用 (d) + (e) 代替 (c)
            void FuncNotShow(const MyData& md, int& result);                // (e)
    4. 默认值
        如果使用默认值，导致必须 include 一些文件，则推荐使用重载代替；位于违反下边的第 6.3 条（完整编号 "...B.6.3"）
            void Func(char f, const ClassOrStruct& cd = ClassOrStruct());   // (a) 不推荐：因为须包含 ClassOrStruct，违反 "...B.6.3"
            void Func(char f, const ClassOrStruct& cd);                     // (b) 推荐使用 (b) + (c) 代替 (a)
            void Func(char f);                                              // (c)
    5. 函数形参命名：与函数局部变量命名规范相同，见 A.4。

4. 枚举：的前置声明即处理
    header.h    定义枚举        enum E : int { A = 1 };
    code.h      前置声明枚举    enum E : int;

5. 花括号
    禁止的写法
        if (is_true) return false;
        if (is_true) { return false; }
    允许的写法
        if (is_true)
        {
            return false;
        }

6. 文件包含
    1. 路径名：路径中禁止出现 "\\"，应使用 "/"，如：
        禁止的写法
            #include "project_management\blisk_settings.h"
        允许的写法
            #include "project_management/blisk_settings.h"
    2. 凡涉及网格的类头，类头文件中禁止包含任何 OCCT 类（包括 gp_Pnt），
        一律使用前向声明，这是为了避免：因 pmp Handle 类与 OCC Handle 宏冲突，导致当需要同时包含该类和 pmp 类时必须有特定的先后顺序，使用不便。
    3. 头文件，#include 非必要减少包含。

7. 关于 std::list 的排序：
    不要对 std::list 使用 std::sort 或 stable_sort，
    STL 的排序算法，仅接受随机存储迭代器，
    std::list 自己有 sort 方法。

8. 变量类型转换
    1. 基本数据类型：int、char、double 等，允许使用 C 风格转换，如：
        double a = 1.23;
        int b = int(a);              // 允许
        int c = (int)std::ceil(a);   // 允许
    2. 指针类型
        dynamic_cast 运行时类型信息检查 RTTI：Run-time Type Information
            比 static_cast 会执行额外的 RTTI，效率比 static_cast 低；当且仅当未知其类型时使用
        static_cast 直接强转
        例如：
            void Func(Base* b)
            {
                if (!b) ...

                // [方式一，小心使用]直接强制转换，不执行 RTTI，由程序员保证转换的安全性
                // 若 b 非空，则 d 必非空；但 b 可能不是 Derived 类型，无效指针
                Derived* d = static_cast<Derived*>(b);
                if (d) ... // 由于无法判断 d 是否有效（非空不代表有效），逻辑可能错误

                // [方式二，非必要不使用] 执行 RTTI，安全转换
                // 若 b 不是 Derived 类型，得空指针
                Derived* d = dynamic_cast<Derived*>(b);
                if (d) ... // 一定是安全的

                // [方式三，推荐使用]主动判断类型后强制转换
                if (b->IsDerivedType())                     // 即由程序员判断转换的安全性
                {
                    Derived* d = static_cast<Derived*>(b);  // 转换为确定的类型
                    ...
                }
            }

9. 函数行数、每行字符数
    推荐
        每个函数代码行数 ≤ 100
        每行代码字符个数 ≤ 120
    优先按“推荐”编码，特殊必要情况下无法满足“推荐”要求时，应向代码评审者陈述理由。

10. 命名空间
    1. 项目代码禁止直接手写 namespace qianjizn::xxx，优先使用项目定义的命名空间宏。
    2. 已有模块级宏时，优先使用模块级宏，如：
        QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN / QJ_NAMESPACE_FIT_CLOUD_SERVER_END
        QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN / QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
    3. 仅在没有模块级宏，或仅做少量前向声明时，允许使用通用宏，如：
        QJ_NAMESPACE_BEGIN1(cloudserver)
        class UserAuthService;
        QJ_NAMESPACE_END1
    4. 头文件中禁止使用 using namespace xxx。


C. 逻辑规范

1. 关于递归逻辑
    1. 递归是一种逻辑简单的编码方式，但同时也要求对代码执行逻辑有相当透彻的理解。
    2. 提醒各开发者：轻易不要使用递归，确需使用的，应
        1. 在代码中详述执行逻辑；
        2. 提交单元测试；
        3. 向评审者详述代码执行逻辑。

2. 编译、链接时出现任何警告信息，应尽力消除。
    如不恰当的类型转换：
        double b = 1.23;
        int a = b;         // 高精度向低精度，隐式转换，禁止
        int a = (int)b;    // 高精度向低精度，显式转换，允许
        double c = a;      // 低精度向高精度转换，允许







