# LLVM IR գեներացիայի աշխատանքային պլան

## Նպատակը

Սեմանտիկորեն վավեր `Program` AST-ից կառուցել ստուգված LLVM IR։ Գեներատորը
անուններ կամ տիպեր նորից չի լուծում․ այն օգտագործում է նույն `SymbolTable`-ն ու
`SemanticModel`-ը, որոնցով ծրագիրը վերլուծվել է։ Սխալ կամ չվերլուծված AST-ի
փոխանցումը ծրագրավորման սխալ է։

Առաջին նպատակը ընթեռնելի `.ll` ֆայլ ստանալն է։ Object file-ի ստեղծումը,
runtime-ի հետ կապակցումն ու executable-ի գործարկումը կավելացվեն վավեր IR
գեներացիայից հետո։ Ամեն փուլ պետք է ավարտվի աշխատող build-ով և համապատասխան
թեստերով։

```text
Scanner → Parser → AST → SemanticAnalyzer
                         ↓
            SymbolTable + SemanticModel
                         ↓
                  LLVM IR Generator
                         ↓
                llvm::verifyModule
                         ↓
                  .ll / object file
                         ↓
                 avium-runtime
```

## Repository-ի ներկա վիճակը

- Parser-ը կառուցում է ամբողջ լեզուն ներկայացնող `Program` AST։
- `SemanticAnalyzer`-ը լուծում է անուններն ու տիպերը և ստուգում կանչերը,
  control flow-ն ու `RETURN`-ը։
- `SemanticModel`-ը պահում է `NodeId → SymbolId`, expression type և entry-point
  կապերը։
- `SymbolTable`-ը տարբերակում է local, parameter ու `FOR` փոփոխականները և
  ենթածրագրերը։
- AST-ն ընդգրկում է անհրաժեշտ control-flow հանգույցները, զանգվածները, կանչերն
  ու վերադարձը։
- `avium-runtime`-ը կառուցվում է որպես առանձին C17 static library և
  իրականացնում է տեքստերը, զանգվածները, I/O-ն, `NUM`, `STR`, `LEN`, `SQR` ու
  runtime սխալները։
- Root CMake project-ը միացնում է C-ն, C++-ը, `runtime/` ենթապանակը և LLVM-ի
  Core/Support բաղադրիչները՝ ուղղակի `prunus` ու test executable target-ներին։
- Նվազագույն `IRCodeGen`-ը դատարկ Կեռասի `Main`-ից ստեղծում է host target
  triple-ով LLVM module, ներքին Կեռասի procedure և C ABI-ի `i32 @main()`
  wrapper, ապա module-ը ստուգում է `llvm::verifyModule()`-ով։ Հաջող semantic
  analysis-ից հետո CLI-ն կանչում է generator-ը և module-ը տպում `stdout`։
  `RuntimeAbi`-ն դեռ իրականացված չէ։
- `book/ch07-llvm-ir.md`-ը դեռ չի պարունակում backend-ի տեխնիկական
  նկարագրությունը։

## Մինչև generator-ը պարտադիր պայմանավորվածությունները

### Runtime ABI

Այս մասը կայունացված է։ Public C ABI-ում `avium_text`-ը երբեք արժեքով չի
փոխանցվում կամ վերադարձվում։ Runtime-ի մուտքային տեքստերը pointer-ներ են, իսկ
նոր տեքստ վերադարձնող գործողությունները ստանում են առաջին
`avium_text* result` պարամետրը։ Այսպես backend-ը չի իրականացնում
target-specific `byval` կամ `sret` lowering։ Օրինակ՝

```c
void avium_input(avium_text* result, unsigned line);
void avium_text_concat(
    avium_text* result,
    const avium_text* left,
    const avium_text* right,
    unsigned line);
```

LLVM module-ի runtime հայտարարությունները պետք է ճշգրտորեն կրկնեն
`runtime/include/*.h`-ի ստորագրությունները։

### Builtin-ների ինքնությունը

Semantic analyzer-ը ճանաչում է `Print`, `Input`, `NUM`, `SQR`, `STR` և `LEN`
builtin-ները, ներառյալ `STR(BOOL)` և `LEN(TEXT|array)` տարբերակները։ Մինչև
codegen-ը ցանկալի է `SubroutineSymbol::builtin` boolean-ը փոխարինել
`BuiltinKind`-ով։ Այդ դեպքում generator-ը builtin lowering-ը կընտրի semantic
identity-ով, ոչ թե անվան տողը համեմատելով։

### Dynamic `DIM`-ի lifecycle-ը

Array slot-ը ստեղծվում է ենթածրագրի entry block-ում, բայց զանգվածը հատկացվում է
հենց `DIM`-ի կատարման պահին։ Եթե նույն `DIM`-ը կրկին կատարվում է, օրինակ՝
ցիկլում, նախ ոչնչացվում է հին զանգվածը, ապա ստեղծվում է նորը։ Տեղային զանգվածը
ոչնչացվում է ենթածրագրից դուրս գալիս, իսկ borrowed array parameter-ը՝ ոչ։

### Կեռասի ենթածրագրերի ABI-ն

- `REAL` պարամետրն ու վերադարձվող արժեքը LLVM `double` են։
- `BOOL` պարամետրն ու վերադարձվող արժեքը LLVM `i1` են։
- `TEXT` պարամետրը փոխանցվում է storage-ի `ptr`-ով։ Callee-ն entry block-ում
  `avium_text_copy`-ով ստեղծում է անկախ տեղային պատճեն՝ պահպանելով լեզվի value
  semantics-ը։
- `TEXT` վերադարձնող ֆունկցիան LLVM-ում վերադարձնում է `void` և առաջին
  պարամետրով ստանում կանչողի հատկացրած `avium_text* result` storage-ը։ Կանչից
  առաջ այն գրելի և ownership չունեցող է, իսկ հաջող վերադարձից հետո ownership-ը
  պատկանում է կանչողին։ Սա Կեռասի բացահայտ ABI-ն է, ոչ LLVM `sret`։
- Array parameter-ը borrowed `avium_array*` է և callee-ում չի ոչնչացվում։

## LLVM տիպերի մոդելը

| Կեռաս | LLVM հաշվարկային տիպ | Պահպանում/runtime |
| --- | --- | --- |
| `REAL` | `double` | `double` |
| `BOOL` | `i1` | Runtime ABI սահմանին ըստ C ստորագրության |
| `TEXT` | հասցեով կառավարվող `%avium.text = type { ptr, iN, i8 }` | `avium_text` |
| զանգված | `ptr` | opaque `avium_array*` |
| պրոցեդուրա | `void` | `void` |
| `REAL`/`BOOL` ֆունկցիա | համապատասխան scalar | անմիջական return value |
| `TEXT` ֆունկցիա | `void` | առաջին `ptr result` պարամետր |

`%avium.text`-ը named, ոչ packed կառուցվածք է։ Դաշտերի ֆիքսված ինդեքսներն են
`0`՝ `data`, `1`՝ `length`, `2`՝ `owned`։ `iN`-ը թիրախի C ABI-ի `size_t`
լայնությամբ integer type-ն է և չի ամրագրվում որպես `i64`։ Runtime header-ում
չկան կառուցվածքի չափը, alignment-ը, field offset-ները կամ `size_t`-ի լայնությունը
ստուգող compile-time assertions։

`owned`-ի C տիպը `bool` է։ Ընթացիկ աջակցվող C ABI-ում այն memory-ում մեկ բայթ է,
ուստի LLVM storage-ի դաշտը `i8` է՝ canonical `0` կամ `1` արժեքով, ոչ թե `i1`։
Struct-ը packed չէ և ձեռքով padding չի ստանում։ Offset-ները, allocation size-ն
ու ABI alignment-ը հաշվարկվում են `DataLayout::getStructLayout()`-ով, իսկ
դաշտերը հասցեագրվում են typed GEP-ով։ Սկզբնական backend-ը գեներացնում է միայն
կապակցվող runtime-ի նույն թիրախի համար։ C layout-ի ու LLVM layout-ի
համատեղելիությունը պետք է հաստատել առանձին ABI integration test-ով։ Նոր թիրախ
ավելացնելիս չի կարելի `size_t`-ը կամ C `bool`-ի storage-ը ենթադրել միայն pointer
width-ից։

Օգտատիրոջ `Main` ենթածրագրի կողքին գեներացվում է C ABI-ի `i32 @main()` wrapper,
որը կանչում է semantic model-ում նշված entry point-ը և վերադարձնում `0`։ User
subroutine-ների անունները պետք է անվտանգ mangling ունենան, որպեսզի չբախվեն
runtime-ի ու համակարգային անուններին։

## Codegen-ի կանոնները

- Generator-ին փոխանցվում է միայն հաջող semantic analysis անցած `Program`-ը՝
  նույն `SymbolTable`-ի և `SemanticModel`-ի հետ։
- Անունների լուծումը, տիպերի ստուգումը, `Main`-ի գոյությունն ու
  ստորագրությունների վավերությունը semantic analyzer-ի պատասխանատվությունն են։
  Codegen-ը դրանք չի կրկնում և Կեռասի semantic diagnostic չի ստեղծում։
- `ScalarType::Name`-ի վավեր արժեքներն են `Bool`, `Real` և `Text`։ Հաջող
  semantic analysis-ից հետո օգտագործվող ամեն expression ունի կոնկրետ տիպ, իսկ
  անհնար enum ճյուղերը նշվում են `std::unreachable()`-ով։
- Parser-ը առանց տիպի `DIM` կամ parameter չի ավելացնում AST-ին, ուստի codegen-ը
  դրանց տիպի գոյությունը չի ստուգում։
- AST node-երը, variable storage-ները և callee-ները կապվում են `SymbolId`-ով։
  Codegen-ը դրանց անունով lookup չի կատարում։ Entry point-ը նույնպես վերցվում է
  `SemanticModel`-ի `SymbolId`-ով։
- `SemanticModel`-ի պարտադիր կապերի բացակայությունը generator-ի նախապայմանի
  խախտում է, ոչ թե աղբյուր ծրագրի diagnostic։
- Array/scalar և function/procedure տարբերակումները ընտրում են արդեն վավեր
  հանգույցի LLVM ներկայացումը և semantic ստուգումներ չեն։
- Ամեն module անցնում է `llvm::verifyModule()`։ Ձախողումը compiler-ի ներքին
  սխալ է։
- Մինչև CLI-ի ելքային ռեժիմների իրականացումը հաջող pipeline-ը ամբողջ LLVM IR-ը
  տպում է `stdout`։

## Իրականացման փուլերը

### Փուլ 1․ LLVM build և generator-ի արտաքին ինտերֆեյս

- Կատարված է՝ CMake-ը `find_package(LLVM CONFIG REQUIRED)`-ով գտնում է LLVM-ը։
  Core/Support-ը, include path-երն ու LLVM-ի պահանջած compile definitions-ը
  ուղղակի ավելացվում են `prunus` և test executable target-ներին։
- Compiler-ի production աղբյուրները պահել մեկ `prunus` executable target-ում։
  Առանձին `avium-frontend` կամ `avium-ir` library target չի ստեղծվում։
- LLVM dependency-ն կապել `prunus`-ին, իսկ codegen թեստերն ավելացնելուց հետո՝
  նաև գոյություն ունեցող test executable-ին։ `avium-runtime`-ը շարունակում է
  ինքնուրույն C17 գրադարան կառուցվել։
- Ավելացված են՝

  ```text
  src/ircodegen.hxx
  src/ircodegen.cxx
  src/runtimeabi.hxx
  src/runtimeabi.cxx
  ```

- `IRCodeGen`-ն ընդունում է `LLVMContext`, `SymbolTable` և `SemanticModel`, իսկ
  `generate()`-ը՝ `Program`։ AST-ը LLVM-specific տվյալներով չի փոփոխվում, և
  արդյունքը `std::unique_ptr<llvm::Module>` է։
- Կատարված է՝ առաջին թեստը դատարկ `Main`-ից ստանում է վավեր module, ներքին
  Կեռասի procedure և C entry point։

### Փուլ 2․ module, տիպեր և ստորագրություններ

- Նախ սահմանել target triple-ն ու data layout-ը, ապա կառուցել `%avium.text`
  named struct-ը։ ABI integration test-ով համեմատել C struct-ի field offset-ները,
  size-ն ու alignment-ը LLVM `StructLayout`-ի արդյունքի հետ։
- Մեկ տեղում իրականացնել `ScalarType::Name → llvm::Type` փոխակերպումն ու
  runtime ABI-ի բոլոր LLVM հայտարարությունները։
- Նախապես հայտարարել user subroutine-ները, որպեսզի աշխատեն forward և recursive
  կանչերը։ LLVM անունները կառուցել `SymbolId`-ով կամ անվտանգ mangling-ով։
- Պահել `SymbolId → llvm::Function*` և `SymbolId → Storage` քարտեզները։
- Գեներացնել C `main` wrapper-ը՝ առանց AST-ում `Main` անունը նորից որոնելու։
- User subroutine-ի համար ստեղծել entry block, local slot-եր և սկզբնական
  արժեքներ՝ `FALSE`, `0.0`, դատարկ text և `null` array descriptor։
- `REAL`/`BOOL` parameter-ները store անել արժեքով, `TEXT` parameter-ը պատճենել
  փոխանցված հասցեից, իսկ array parameter-ը պահել borrowed reference։
- `TEXT` return type ունեցող ստորագրությանը ավելացնել առաջին result pointer-ը։
- Ամեն `RETURN` արժեքը պահում կամ տեղափոխում է return/result storage, ապա անցնում
  ընդհանուր cleanup block։ Cleanup-ից հետո scalar function-ը կատարում է `ret`,
  իսկ procedure-ն ու `TEXT` function-ը՝ `ret void`։
- Թեստավորել procedure, scalar/text parameter ու return, առաջ ուղղված
  հայտարարում և recursion։

### Փուլ 3․ scalar արտահայտություններ և վերագրումներ

- Գեներացնել `BOOL`, `REAL` և text literal-ները։ Text literal-ի բայթերը պահել
  module-ի private constant storage-ում և կազմել borrowed `%avium.text` handle։
- Իրականացնել scalar `Variable` load-ը և `LET` store-ը։
- Իրականացնել unary `+`, `-`, `NOT` գործողությունները։
- Իրականացնել թվաբանական գործողությունները։ `/`-ը floating-point բաժանում է,
  `\`-ը՝ դեպի զրո կլորացված քանորդ, `MOD`-ը՝ մնացորդ, `^`-ը՝ runtime/libm
  `pow` կանչ։
- Իրականացնել numeric ու boolean equality/comparison-ները։
- `\`, `MOD`, `^` և floating-point comparison-ների համար առանձին թեստերով
  ամրագրել `NaN`, infinity և signed zero վարքը։
- Ամեն ենթափուլի թեստը և՛ verify է անում module-ը, և՛ ստուգում հիմնական
  instruction-ները։

### Փուլ 4․ control flow

- `IF`/`ELSEIF`/`ELSE`-ի համար կառուցել condition, branch և merge block-եր։
  Դատարկ branch-ը նույնպես ճիշտ է ավարտվում, իսկ `RETURN`-ով ավարտված branch-ին
  երկրորդ terminator չի ավելացվում։
- `WHILE`-ի համար կառուցել condition, body և exit block-եր։
- `AND` և `OR` գործողությունները գեներացնել short-circuit branch-երով ու PHI
  արժեքով, ոչ bitwise instruction-ով։
- `FOR`-ի begin, end և step expression-ները հաշվարկել ճիշտ մեկ անգամ։ Դրական
  քայլի դեպքում կիրառել `<=`, բացասականի դեպքում՝ `>=`, ապա մարմնից հետո
  մեծացնել հաշվիչը։
- Թեստավորել nested control flow-ը, early return-ը և այն դեպքերը, երբ branch-ը
  կամ loop body-ն ենթածրագրի վերջին statement-ն է։

### Փուլ 5․ user subroutine-ների կանչեր

- `CALL`-ը գեներացնել որպես `void` կանչ։ `REAL`/`BOOL` վերադարձնող `Apply`-ն
  օգտագործում է call-ի անմիջական արդյունքը։
- `TEXT` վերադարձնող `Apply`-ի համար caller-ը հատկացնում է դեռ ownership
  չունեցող temporary storage և այն փոխանցում որպես առաջին result pointer։
- `REAL`/`BOOL` argument-ները փոխանցել արժեքով, `TEXT` argument-ները՝ storage-ի
  հասցեով, իսկ array-ները՝ borrowed descriptor pointer-ով։
- Callee-ն գտնել `SemanticModel`-ի `SymbolId`-ով և ապահովել recursive ու mutual
  կանչերը։
- Թեստավորել բազմաթիվ argument-ներ, return value-ն և function result-ի
  անմիջական օգտագործումն ավելի մեծ expression-ում։

### Փուլ 6․ runtime, builtin-ներ և `TEXT` ownership

- Օգտագործել `runtime/include/*.h`-ում սահմանված C ABI-ն՝ text-ի ստեղծման,
  պատճենման, տեղափոխման, ոչնչացման, միակցման, համեմատման և input-ի համար։
- `&`, text `=`, `<>`, `<`, `<=`, `>` և `>=` գործողություններն իջեցնել runtime
  կանչերի։ Համեմատությունը բովանդակությամբ է, ոչ pointer-ով։
- Նույն ABI-ով իջեցնել `Input`, `NUM`, `STR`, `LEN` և `SQR` builtin-ները՝ source
  line փոխանցելով այն runtime ֆունկցիաներին, որոնց ստորագրությունը դա պահանջում
  է։
- `Print`-ն ընդունում է միայն `TEXT` և իջեցվում է `avium_print_text` runtime
  կանչի։ `BOOL` կամ `REAL` արժեք տպելու համար Կեռասի ծրագիրը նախ օգտագործում է
  `STR`։
- Պահպանել `runtime/design.md`-ի ownership պայմանագիրը․
  - text literal-ը borrowed է,
  - `Input`, `STR`, concatenation և text-returning function-ը owned արժեք են
    տալիս,
  - borrowed RHS-ը variable-ին վերագրելիս կիրառվում է `avium_text_copy`,
  - owned temporary-ն տեղափոխելիս կիրառվում է `avium_text_move_assign`,
  - `TEXT` վերադարձնելիս ownership-ը փոխանցվում է caller-ի result storage-ին,
  - ամեն owned local կամ temporary ոչնչացվում է ճիշտ մեկ անգամ։
- Cleanup-ում ոչնչացնել local text-երն ու local array-ները, բայց ոչ borrowed
  array parameter-ները։ Բոլոր `RETURN`-ները տանել նույն cleanup epilogue։
- Թեստավորել overwrite/self-assignment-ը, owned/borrowed argument-ները, early
  return-ը և բոլոր control-flow ուղիների cleanup-ը։

### Փուլ 7․ զանգվածներ և runtime ստուգումներ

- `DIM`-ի size expression-ը հաշվարկել ճիշտ մեկ անգամ։ Կրկնակի կատարման դեպքում
  նախ ոչնչացնել slot-ի հին descriptor-ը, ապա կանչել `avium_array_create`-ը՝
  element tag-ով ու source line-ով։
- Չափի ամբողջ, դրական ու ներկայացնելի լինելը, allocation-ը և element-ների
  սկզբնարժեքավորումը թողնել runtime-ի պայմանագրին։
- `a[i]`-ի հասցեն ստանալ `avium_text_array_at`, `avium_real_array_at` կամ
  `avium_bool_array_at` typed accessor-ով։ Runtime-ն է ստուգում descriptor-ը,
  element type-ը, index-ի ամբողջ լինելն ու սահմանները։
- Տեղային array-ները ոչնչացնել ենթածրագրից դուրս գալու բոլոր ուղիներում։ Array
  parameter-ը չոչնչացնել և փոխանցել հղումով, որպեսզի element-ի փոփոխությունը
  տեսանելի լինի caller-ին։
- `LEN(array)`-ն իջեցնել runtime-ի array length գործողության։
- Թեստավորել dynamic և կրկնվող `DIM`, բոլոր element type-երը, parameter-ով
  փոխանցումը և size/index runtime սխալները։

### Փուլ 8․ CLI, object file և ամբողջական ինտեգրում

- Սկզբում ավելացնել հստակ IR ռեժիմներ՝

  ```text
  prunus --emit-ast source.bas
  prunus --emit-llvm source.bas
  prunus --emit-llvm -o output.ll source.bas
  ```

- Pipeline-ը դարձնել scanner → parser → semantic analyzer → IR generator →
  verifier → output։ Parse կամ semantic սխալի դեպքում codegen չգործարկել։
- Object file ու executable ստեղծելը ավելացնել հաջորդ քայլով՝ `-c` կամ
  `--compile` ռեժիմով, ապա կապել `avium-runtime`-ի հետ։
- Բոլոր `examples/*.bas` ֆայլերի համար ավելացնել IR smoke test։ Քանի որ օրինակների
  մի մասը կարող է դիտավորյալ diagnostic ակնկալել, դրանք բաժանել
  expected-success և expected-diagnostic խմբերի։
- Ընտրված փոքր ծրագրերը գործարկել JIT-ով կամ native executable-ով և ստուգել
  stdout-ը, stderr-ը ու exit status-ը։
- Implementation-ին զուգահեռ լրացնել `book/ch07-llvm-ir.md`-ը։

## Թեստավորման ռազմավարություն

- `tests/ircodegen_test.cxx`՝ փոքր `Program` → LLVM module։
- Ամեն գեներացված module-ի համար պարտադիր `llvm::verifyModule()`։
- Կառուցվածքային թեստեր՝ function signature, `%avium.text` layout, basic block,
  runtime call, PHI և short-circuit։
- Խուսափել LLVM version-ից կախված մեծ IR snapshot-ներից։ Նախընտրել verifier,
  կառուցվածքային ստուգումներ և փոքր կայուն IR հատվածներ։
- End-to-end դեպքեր՝ `.bas → .ll → object → runtime link → execution`։
- Պարտադիր եզրային դեպքեր՝ early return, text overwrite/self-assignment,
  owned/borrowed text argument, text return, array parameter mutation, invalid
  size/index, positive/negative `FOR STEP`, short-circuit-ի չկատարվող աջ կողմ,
  `NaN`, infinity, signed zero, recursive և forward call։
- AddressSanitizer/LeakSanitizer end-to-end թեստեր՝ text/array ownership-ի համար։
- Runtime ABI-ի C թեստերը շարունակել առանձին և ավելացնել սահմանային արժեքների
  ու error exit-երի դեպքեր։

## Առաջարկվող փոփոխությունների բաժանումը

1. `BuiltinKind` և runtime compatibility-ի լրացուցիչ թեստեր։
2. LLVM/CMake skeleton, target configuration, type mapping և subroutine ABI։
3. Scalar expression-ներ, control flow ու scalar builtin-ներ։
4. `TEXT` գործողություններ, ownership, cleanup և text builtin-ներ։
5. Զանգվածներ, կրկնվող `DIM` և `LEN(array)`։
6. CLI, object generation, runtime linking և end-to-end թեստեր։
7. `book/ch07-llvm-ir.md` փաստաթղթավորում։

## Ավարտված լինելու չափանիշները

Պլանը ավարտված է, երբ Կեռասի բոլոր գործողությունները, control-flow
կառուցվածքները, user subroutine-ները, builtin-ները և զանգվածները ստանում են
վավեր LLVM IR, ownership/cleanup-ը ճիշտ է բոլոր կատարման ուղիներում, հաջող
օրինակները հասնում են codegen ու execution փուլերին, runtime սխալներն ունեն
կանխատեսելի վարք, իսկ compiler-ի և runtime-ի ամբողջ CTest փաթեթն անցնում է։
