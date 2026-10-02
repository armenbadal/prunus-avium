# LLVM և կոդի գեներացիա

Կոդի գեներատորը պետք է Կեռասի ծրագրից կառուցի LLVM IR-ի մոդուլ։ C++ իրականացման
տեսակետից՝ սեմանտիկ ստուգում անցած վերացական շարահյուսական ծառից պետք է կառուցել
`llvm::Module` օբյեկտ, որում Կեռասի ամեն մի ենթածրագրի համար ստեղծված է համարժեք
`llvm::Function` օբյեկտ։

Բայց մինչ բուն Կեռասի կոդի թարգմանությանը հասնելը, նախ երկու փոքր օրինակով
ծանոթանանք LLVM գրադարանի այն մի քանի առանցքային օբյեկտներին, որոնք
օգտագործվելու են կոդի գեներացիայի ամբողջ ընթացքում։

Առաջին օրինակը. գրել C++ կոդ, որը LLVM գրադարանի օգտագործմամբ IR է գեներացնում
մինիմալ Կեռաս ծրագրի համար։

```cerasus
SUB Main
END SUB
```

Ո՞րն է սրա համարժեք IR-ը։ Ես հաճախ մի հնարք եմ օգտագործում, որը շատ է օգնում LLVM
գրադարանի նրբություններն ու հնարավորությունները հասկանալու համար։ Այդ հնարքը հետևյալն
է. գրում եմ ծրագիրը Սի լեզվով (որպես մի պարզ լեզու), ապա `clang`-ով ստանում եմ դրա
IR-ը, հետո սկսում եմ C++-ով ծրագրավորել այդ IR-ը գեներացնող կոդը։

Ցուցադրեմ քայլ առ քայլ։ Վերը բերված մինիմալ Կեռաս ծրագրի Սի համարժեքը `ex00.c` ֆայլում
գրված հետևյալ կոդն է.

```c
// Կեռաս ծրագրի մուտքի կետը
void cerasus_Main()
{}

// Սի ծրագրի մուտքի կետը, որում կանչվում
// է Կեռաս ծրագրի մուտքի կետը
int main(void)
{
    cerasus_Main();
    return 0;
}
```

Սրանից LLVM IR ստանալու համար պետք է `clang` գործիքը կիրառել հրամանային տողի `-S` և
`-emit-llvm` արգումենտներով։ Այսպես.

```bash
$ clang -S -emit-llvm -O0 -fno-ident -fno-pic ex00.c
```

Արդյունքում ստեղծվում է `ex00.ll` ֆայլը՝ հետևյալ բովանդակությամբ։

```llvm
; ModuleID = 'ex00.c'
source_filename = "ex00.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @cerasus_Main() #0 {
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  call void @cerasus_Main()
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 2}
!2 = !{i32 7, !"frame-pointer", i32 2}
```

Մի կողմ կթողնեմ այս տեքստում հայտնված հավելյալ ծառայողական տեղեկությունը
ու կկենտրոնանամ երկու ֆունկցիաների սահմանումների վրա։ Ահա Կեռասի `Main`
ենթածրագրի համարժեքը.

```llvm
define dso_local void @cerasus_Main() #0 {
  ret void
}
```

Եվ ահա Սի ծրագրի `main` ֆունկցիայի թարգմանությունը, որում կանչված է
`cerasus_Main`-ը։

```llvm
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  call void @cerasus_Main()
  ret i32 0
}
```

Քանի որ `clang`-ը կանչվել է `-O0` (ոչ մի օպտիմիզացիա) արգումենտով, այս ֆունկցիայում
մնացել են որոշ ավելորդ հրահանգներ։ Բայց ինձ համար կարևորը `call void @cerasus_Main()`
տողի առկայությունն է։

Իհարկե, արհեստական բանականության գործիքներն օգտագործելով կարելի է նման արդյունք
ստանալ ու ավելի արագ առաջ գնալ։ Սակայն ես խորհուրդ եմ տալիս, գոնե LLVM համակարգին
ծանոթանալու փուլում, անպայման ձեռքով գրել կոդն ու փորձել պարզել ու հասկանալ ամեն
մի տողի ու արտահայտության իմաստը։

Ուրեմն, նպատակն արդեն ավելի հստակ է. C++-ով ու LLVM գրադարանների օգտագործմամբ գրել
մի ծրագիր, ասենք՝ `generate_ex00`, որի կատարումը կարտածի նույն (կամ համարյա նույն)
արդյունքը, ինչ ստեղծել է `clang`-ը `-S -emit-llvm` արգումենտներով։ Պետք է ստեղծել
`llvm::Module` օբյեկտ, որում երկու `llvm::Function` օբյեկտներ են. մեկը
`cerasus_Main`-ի համար, մյուսը՝ `main`-ի։ Առաջինի մարմինը դատարկ է, երկրորդում երկու
հրաման է. `cerasus_Main()`-ի կանչը և `return 0`-ն։

Քանի որ սա մի պարզ, գծային վարժություն է, ամբողջ կոդը կգրեմ `main`
ֆունկցիայում։

```c++
int main()
{
```

Առաջին ու ամենակարևոր LLVM օբյեկտը `LLVMContext`-ն է։ Սա, ինչպես երևում է անունից,
LLVM IR օբյեկտների ընդհանուր միջավայրն է. դրա հետ են կապված տիպերի ու 
հաստատունների եզակի ներկայացումները, ախտորոշման (diagnostics) հնարավորությունները 
և LLVM-ի ներքին որոշ ընդհանուր տվյալներ։ `LLVMContext`-ի կիրառությանը կհանդիպենք 
շատ քայլերում ու ամեն մի կետում ես կնշեմ, թե այն ինչ տեղեկություն է տալիս այդ պահին։
Հիմա ընդամենը սահմանում եմ `context` օբյեկտը։

```c++
    llvm::LLVMContext context;
```

Հաջորդ կարևոր օբյեկտը `IRBuilder`-ն է։ Նորից անունը հուշում է, որ սա IR կառուցելու
համար է։ Այս օբյեկտի միջոցով են ստեղծվելու բոլոր IR հրահանգները, օրինակ, վերը
հիշատակված ֆունկցիայի կանչը և ֆունկցիայից արժեք վերադարձնող `return`-ը։ Սա էլ
սահմանեմ.

```c++
    llvm::IRBuilder<> builder{context};
```

Մոդուլը ֆունկցիաների, գլոբալ տվյալների, տիպերի ու մետատվյալների կոնտեյներ է։
Այն, ըստ էության, մոդելավորում է կոմպիլյացիայի միավորը։ `Module` օբյեկտ
ստեղծելիս պետք է տալ նրա անունը. սովորաբար դա այն ֆայլի անունն է, որի համար
կառուցվում է LLVM մոդուլը։ Պետք է տալ նաև տվյալ պահին օգտագործվող կոնտեքստը։

```c++
    llvm::Module module{"ex00", context};
    module.setTargetTriple(llvm::sys::getDefaultTargetTriple());
```

Մոդուլի օբյեկտին կիրառված `setTargetTriple` մեթոդը սահմանում է թիրախային եռյակը՝
ճարտարապետության, օպերացիոն համակարգի և կատարման միջավայրի նույնականացումը։
Առայժմ օգտագործել եմ համակարգի `getDefaultTargetTriple`-ը. հետագայում, երբ կարիք 
լինի, կանդրադառնամ սրան։

Հիմա պետք է ստեղծեմ դատարկ մարմնով `cerasus_Main` ֆունկցիան։ Քանի որ այն ոչինչ չի
անում և վերադարձնում է `void`, նրա միակ basic block-ը կավարտվի `ret void`
հրահանգով։ Նախ պետք է ստեղծել `cerasus_Main`-ի տիպը. պարամետրեր չունի,
վերադարձնում է `void`։ LLVM տիպերի համակարգի `getVoidTy` ֆունկցիայով վերցնում եմ 
`void` տիպը ներկայացնող օբյեկտի ցուցիչը՝ նշելով կոնտեքստը։ Հետո `FunctionType::get` 
ֆունկցիայով կառուցում եմ իմ ֆունկցիայի տիպը։ Այստեղ `FunctionType::get`-ը ստանում 
միայն ֆունկցիայի վերադարձվող արժեքի տիպը և `false` արժեքը. վերջինս ցույց է տալիս, 
որ այս ֆունկցիան վարիադիկ չէ։

```c++
    auto* voidTy = llvm::Type::getVoidTy(context);
    auto* cerasusMainFuncType = llvm::FunctionType::get(voidTy, false);
```

LLVM-ի ֆունկցիան ստեղծում եմ `Function::Create` ֆունկցիայով, որին տալիս եմ
նախորդ քայլում կառուցված ֆունկցիայի տիպը, տեսանելիության պարամետրը, տվյալ դեպքում՝
`ExternalLinkage`, ֆունկցիայի անունը և այն մոդուլի հղումը, որին պատկանում է
ֆունկցիան։

```c++
    auto* cerasusMainFunc = llvm::Function::Create(cerasusMainFuncType,
                      llvm::Function::InternalLinkage, "cerasus_Main", module);
```

Հիմա պետք է `cerasus_Main` ֆունկցիայի մարմնում գրեմ միակ `ret void` հրահանգը։
Ֆունկցիայի մարմինը `BasicBlock`-ների շարք է։ `BasicBlock::Create`-ով սարքում
եմ `cerasus_Main` ֆունկցիայի առաջին անանուն բլոկը, ու `builder`-ին ասում եմ,
որ վերջինս դարձնի IR հրահանգները գրելու ընթացիկ կետ։

```c++
    auto* cerasusMainBegin = llvm::BasicBlock::Create(context, "", cerasusMainFunc);
    builder.SetInsertPoint(cerasusMainBegin);
```

Իսկ հետո `builder`-ի միջոցով ընթացիկ basic block-ում ավելացնում եմ `ret void` հրահանգը։

```c++
    builder.CreateRetVoid();
```

Այս կետում `cerasus_Main` ֆունկցիան պատրաստ է։ Հաջորդը պետք է ստեղծեմ `main`
ֆունկցիան ու դրա մեջ կանչեմ `cerasus_Main`-ը։

C լեզվի այս `int`-ին `clang`-ի ստացած IR-ում համապատասխանում է `i32`, ուստի
կառուցում եմ `i32` տիպը։

```c++
    auto* int32Ty = llvm::Type::getInt32Ty(context);
    auto* siMainFuncType = llvm::FunctionType::get(int32Ty, false);
```

Քանի որ `main`-ը կառուցվելիք մոդուլի մուտքի կետն է, այսինքն այն պետք է տեսանելի
լինի կապերի խմբագրիչին (linker-ին), `Function::Create`-ով ֆունկցիան սարքում եմ
`ExternalLinkage` պարամետրով։

```c++
    auto* siMainFunc = llvm::Function::Create(siMainFuncType,
                  llvm::Function::ExternalLinkage, "main", module);
```

Հետո, ինչպես արեցի `cerasus_Main`-ի համար, ստեղծում եմ անանուն basic block և
`builder`-ին ասում եմ, որ դա է IR հրահանգները գրելու ընթացիկ տեղը։

```c++
    auto* siMainBegin = llvm::BasicBlock::Create(context, "", siMainFunc);
    builder.SetInsertPoint(siMainBegin);
```

Ֆունկցիաների կանչի համար `builder`-ն ունի `CreateCall` ֆունկցիան, որը ստանում է
կանչվող ֆունկցիան և `Value*`-երով ներկայացված արգումենտների շարքը։
Իմ դեպքում `cerasus_Main`-ի կանչն արգումենտներ չի սպասում։

```c++
    builder.CreateCall(cerasusMainFunc);
```

`main` ֆունկցիան պետք է վերադարձնի `i32` արժեք, տվյալ դեպքում՝ `0`։ Այս դեպքում
օգտագործում եմ `CreateRet` ֆունկցիան, որին փոխանցում եմ զրո հաստատունի ցուցիչը։

```c++
    auto* zero = builder.getInt32(0);
    builder.CreateRet(zero);
```

Արտաքուստ ամեն ինչ պատրաստ է, բայց ստեղծված կոդն արտածելուց առաջ ուզում եմ այն
ստուգել LLVM-ի ստանդարտ միջոցներով։ `llvm::verifyModule`-ը սպասում է ստուգվող
մոդուլի հղումը և այն հոսքը, որի վրա պետք է արտածել հայտնաբերված սխալների
հաղորդագրությունները։ Նրա պայմանագիրը մի փոքր անսովոր է. եթե մոդուլը ճիշտ է,
վերադարձնում է `false`, իսկ սխալի դեպքում՝ `true`։ Այդ պատճառով ստուգումը գրված է
հետևյալ կերպ։

```c++
    if( llvm::verifyModule(module, &llvm::errs()) )
        llvm::report_fatal_error("գեներացված մոդուլում սխալ կա");
```

Վերջապես, `llvm::outs` հոսքին արտածում եմ կառուցված մոդուլի տեքստային ներկայացումը։

```c++
    module.print(llvm::outs(), nullptr);

    return 0;
}
```

Լավ է։ Հիմա պետք է լրացնել LLVM-ի պահանջվող վերնագրային ֆայլերը, կոմպիլյացնել ու
կատարել այս օրինակը։

Կատարման արդյունքում ստանում եմ.

```llvm
; ModuleID = 'ex00'
source_filename = "ex00"
target triple = "x86_64-pc-linux-gnu"

define internal void @cerasus_Main() {
  ret void
}

define i32 @main() {
  call void @cerasus_Main()
  ret i32 0
}
```

Առաջին հայացքից ստացվել է `clang`-ով ստացված արդյունքին շատ մոտ արդյունք։ Բայց, 
դրա ֆունկցիոնալության, ավելի ճիշտ՝ ֆունկցիոնալության բացակայության մեջ համոզվելու 
համար, այս արդյունքը ինտերպրետացնենք LLVM-ի IR-ի ինտերպրետատորով։ 

```bash
$ lli ex00.ll
```

Սխալներ չկան ու ոչինչ չի արտածվում։ Սա հենց սպասված արդյունքն է. չէ՞ որ մեր սկզբնական 
ծրագիրը ոչինչ չէր անում, դատարկ `Main` ենթածրագիր էր։

Այս օրինակն իր դատարկ արդյունքով ինձ հնարավորություն տվեց ցուցադրելու LLVM-ի IR 
գեներացնելու առանցքային օբյեկտներն ու մեխանիզմները։ Հիմա, արդեն զինված այս 
գործիքներով, ցույց կտամ թե ինչպես IR ստեղծել աշխարհքին ողջունող C ծրագրի համար։
Այն գրել եմ `ex01.c` ֆայլում։

```c
#include <stdio.h>

int main()
{
    puts("Hello, world!");
    return 0;
}
```

Նորից `clang`-ի `-S` և `-emit-llvm` պարամետրով ստանամ այս ծրագրի IR-ը։ 

```bash
$ clang -S -emit-llvm -O0 -fno-ident -fno-pic ex01.c
```

Ստացվելու է մոտավորապես հետևյալը (կախված կատարման համակարգից ու կոմպիլյատորի 
տարբերակից): Այստեղ ես դեն եմ նետել այս պահին անկարևոր մետատվյալները, որպեսզի
կարողանամ ուշադրությունս կենտրոնացնել էական մանրամասների վրա։

```llvm
source_filename = "ex01.c"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"Ok!\00", align 1

define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = call i32 @puts(ptr noundef @.str)
  ret i32 0
}

declare i32 @puts(ptr noundef) #1
```

Այս կոդում տեսնում եմ երկու անծանոթ միավոր։ Առաջինը՝ գլոբալ տեքստային 
հաստատունի սահմանումն է, երկրորդը՝ `puts` ֆունկցիայի հայտարարությունը։

Այս երկրորդ օրինակի կոդի գեներատորը գրելիս էլ ավելորդ անգամ չեմ կրկնի
արդեն ասվածը։ Ուրեմն, միանգամից սահմանում եմ անհրաժեշտ օբյեկտները.

```c++
int main()
{
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder{context};
    llvm::Module module{"ex02_h.c", context};
    module.setTargetTriple(llvm::sys::getDefaultTargetTriple());
```

Պետք է սահմանեմ գլոբալ տեքստային փոփոխականը որպես 14 նիշերի զանգված։
LLVM-ում նիշերը ներկայացվում են `i8` տիպով։ Ուրեմն նախ ստեղծում եմ `int8Ty`
տիպը, հետո դրա օգտագործմամբ ստեղծում եմ զանգվածի տիպը։

```c++
    auto* int8Ty = llvm::Type::getInt8Ty(context);
    auto* arrayTy = llvm::ArrayType::get(int8Ty, 14);
```

Հետո ստեղծում եմ «Hello, world!» տեքստային հաստատունը ներկայացնող օբյեկտը։

```c++
    auto* hwText = llvm::ConstantDataArray::getString(context, "Hello, world!");
```

Հիմա արդեն կարող եմ սահմանել գլոբալ փոփոխականը՝ վերը սահմանած զանգվածի 
տիպով ու տեքստային հաստատունի արժեքով։

```c++
    llvm::GlobalVariable hwStr{module, arrayTy, true, llvm::GlobalValue::PrivateLinkage, hwText, "hw.str"};
```

Քանի որ տեքստն արտածելու համար `main` ֆունկցիայում կանչելու եմ `puts` ֆունկցիան, 
այստեղ պետք է այն հայտարարեմ (տես `clang`-ի գեներացրած կոդի `declare i32 @puts...` 
տողը)։

```c++
    auto* int23Ty = llvm::Type::getInt32Ty(context);
    auto* ptrTy = builder.getPtrTy();
    auto* putsType = llvm::FunctionType::get(int32Ty, {ptrTy}, false);
    auto* putsFunc = llvm::Function::Create(putsType, llvm::Function::ExternalLinkage, "puts", module);
```

Սա հերիք է `puts`-ի հայտարարության համար։ Հիմա կառուցում եմ `main` ֆունկցիան, ու միանգամից 
ավելացնում եմ առաջին basic block-ը։

```c++
    auto* mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);
    auto* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module);
    auto* mainEntry = llvm::BasicBlock::Create(context, "", main);
    builder.SetInsertPoint(mainEntry);
```

Արդեն կարող եմ կանչել `puts`-ը՝ դրան կանչի արգումենտների ցուցակում տալով գլոբալ տեքստային
հաստատունը ներկայացնող փոփոխականի հասցեն։

```c++
    builder.CreateCall(putsFunc, {&hwStr});
```

Վերադարձնում եմ սովորական դարձած 0 արժեքը։

```c++
    builder.CreateRet(builder.getInt32(0));
```

Ստուգում եմ, որ մոդուլը ճիշտ կառոցված լինի.

```c++
    if( llvm::verifyModule(module, &llvm::errs()) )
        llvm::report_fatal_error("IR code generation produced an invalid module");
```

Եթե ամեն ինչ ճիշտ է, մոդուլը գրում եմ `ex01g.ll` ֆայլում։

```c++    
    std::error_code ec;
    llvm::raw_fd_ostream fout{"ex01g.ll", ec, llvm::sys::fs::OF_None};
    if( ec ) {
        llvm::report_fatal_error("Cannot open file for output");
        return;
    }

    return 0;
}
```

Հիմա նորից պետք է այս ծրագիրը թարգմանել ու կատարել.

```bash
$ clang++ generate_ex01.cxx -o generate_ex01 $(llvm-config --cxxflags --ldflags --libs --system-libs core support)
$ ./generate_ex01
```

Ստացվում է հետևյալ բովանդակությամբ `ex01g.ll` ֆայլը.

```llvm
; ModuleID = 'prunus'
source_filename = "prunus"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [14 x i8] c"Hello, world!\00"

define i32 @main() {
entry:
  %0 = call i32 @puts(ptr @.str)
  ret i32 0
}

declare i32 @puts(ptr)
```

Ամեն ինչ ծանոթ է։ Կարելի է կատարել LLVM-ի բայթկոդի ինտերպրետատորով ու տեսնել արդյունքը.

```bash
$ lli ex01g.ll
Hello, world!
```

Կարծում եմ, որ այս երկու պարզագույն օրինակերն արդեն ընդհանուր պատկերացում տվեցին
այն մասին, թե ինչպես է կազմակերպվում LLVM IR կոդի գեներացիան։ Հաջորդիվ արդեն կսկսեմ
պլանավորել ու իրականացնել վարացական շարահյուսական ծառից կոդ գեներացնելու մոդուլը։
