# LLVM և կոդի գեներացիա

Կոդի գեներատորը պետք է Կեռասի ծրագրից կառուցի LLVM IR-ի մոդուլ։ 
C++ իրականացման տեսակետից՝ սեմանտիկ ստուգում անցած վերացական 
շարահյուսական ծառից պետք է կառուցել `llvm::Module` օբյեկտ, որում 
Կեռասի ամեն մի ենթածրագրի համար ստեղծված է համարժեք `llvm::Function` 
օբյեկտ։

Բայց, մինռ բուն Կեռասի կոդի թարգմանությանը հասնելը, նախ ծանոթանանք 
երկու փոքր օրինակով ծանոթացնեմ LLVM գրադարանի այն մի քանի առանցքային 
օբյեկտներին, որոնք օգտագործվելու են կոդի գեներացիայի ամբողջ ընթացքում։ 

Առաջին օրինակը. գրել C++ կոդ, որը LLVM գրադարանի օգտագործմամբ IR 
է գեներացնում մինիմալ Կեռաս ծրագրի համար։

```cerasus
SUB Main
END SUB
```

Ո՞րն է սրա համարժեք IR-ը։ Ես հաճախ մի հնարք եմ օգտագործում, որը շատ է 
օգնում LLVM գրադարանի նրբություններն ու հնարավորությունները հասկանալու 
համար։ Այդ հնարքը հետևյալն է. գրում եմ ծրագիրը Սի լեզվով (որպես մի պարզ
լեզու), ապա `clang`-ով ստանում եմ դրա IR-ը, հետո սկսում եմ C++-ով 
ծրագրավորել այդ IR-ը գեներացնող կոդը։

Ցուցադրեմ քայլ առ քայլ։ Վերը բերված մինիմալ Կեռաս ծրագրի Սի համարժեքը 
`ex00.c` ֆայլում գրված հետևյալ կոդն է.

```c
// Կեռաս ծրագիր մուտքի կետը
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

Սրանից LLVM IR ստանալու համար պետք է `clang` գործիքը կիրառել հրամանային տողի
`-S` և `-llvm-emit` արգումենտներով։ Այսպես.

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

Մի կողմ կթողնեմ այս տեքստում հայտնված հավելյալ ծառայողական ինֆորրմացիան 
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

Ուրեմն, նպատակն արդեն ավելի հստակ է. C++-ով ու LLVM գրադարանների օգտագործմամբ գրել
մի ծրագիր, ասենք՝ `generate_ex00`, որի կատարումը կարտածի նույն (կամ համարյա նույն)
արդյունքը, ինչ ստեղծել է `clang՝-ը `-S `-emit-llvm` արգումենտներով։ Պետք է ստեղծել 
`llvm::Module` օբյեկտ, որում երկու `llvm::Functrion` օբյեկտներ են. մեկը 
`cerasus_Main`-ի համար, մյուսը՝ `main`-ի։ Առաջինի մարմինը դատարկ է, երկրորդում երկու
հրամանա է. `cerasus_Main()`-ի կանչը և `return 0`-ն։

```c++
void generate_ex00()
{
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder{context};

    auto module = std::make_unique<llvm::Module>("ex00", context);
    module->setTargetTriple(llvm::sys::getDefaultTargetTriple());

    auto* cerasusMainFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
    auto* cerasusMainFunc = llvm::Function::Create(cerasusMainFuncType, llvm::Function::InternalLinkage, "cerasus_Main", *module);
    auto* cerasusMainBegin = llvm::BasicBlock::Create(context, "", cerasusMainFunc);
    builder.SetInsertPoint(cerasusMainBegin);
    builder.CreateRetVoid();

    auto* siMainFuncType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);
    auto* siMainFunc = llvm::Function::Create(siMainFuncType, llvm::Function::ExternalLinkage, "main", *module);
    auto* siMainBegin = llvm::BasicBlock::Create(context, "", siMainFunc);
    builder.SetInsertPoint(siMainBegin);
    builder.CreateCall(cerasusMainFunc);
    builder.CreateRet(builder.getInt32(0));

    module->print(llvm::outs(), nullptr);
}
```
