# LLVM IR գեներացիայի աշխատանքային պլան

## Նպատակը

Սեմանտիկորեն վավեր `Program` AST-ից կառուցել ստուգված LLVM IR։ Գեներատորը
օգտագործում է նույն `SymbolTable`-ն ու `SemanticModel`-ը, որոնցով ծրագիրը
վերլուծվել է, և չի կրկնում անունների լուծումն ու տիպերի ստուգումը։

Առաջին ամբողջական արդյունքը ընթեռնելի `.ll` ֆայլն է։ Object file-ի ստեղծումը,
runtime-ի հետ կապակցումն ու executable-ի գործարկումը հաջորդում են ամբողջ լեզվի
վավեր IR գեներացիային։ Ամեն փուլ ավարտվում է աշխատող build-ով և թեստերով։

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

## Ներկա վիճակը

Frontend-ը և runtime-ը պատրաստ են backend-ի հետ ինտեգրմանը․

- Parser-ը կառուցում է ամբողջ լեզուն ներկայացնող AST։
- `SemanticAnalyzer`-ը լուծում է անուններն ու տիպերը, ստուգում է կանչերը,
  control flow-ն ու `RETURN`-ը և լրացնում `SemanticModel`-ը։
- `SymbolTable`-ը պահում է ենթածրագրերի ստորագրություններն ու local, parameter և
  `FOR` փոփոխականների storage դերը։
- `avium-runtime`-ը առանձին C17 static library է և իրականացնում է տեքստերը,
  զանգվածները, I/O-ն, `NUM`, `STR`, `LEN`, `SQR` ու runtime սխալները։
- CMake-ը գտնում և կապում է LLVM Core/Support-ը compiler-ի ու test executable-ի
  հետ։

`IRCodeGen`-ի հիմքը նույնպես պատրաստ է․

- Constructor-ն ընդունում է `LLVMContext`, `Program`, `SymbolTable` և
  `SemanticModel`։
- `generate()`-ը վերադարձնում է
  `std::expected<std::unique_ptr<llvm::Module>, Error>`։ Codegen-ի ձախողումը չի
  ընդհատում process-ը, այլ վերադարձվում է source line-ով և հաղորդագրությամբ։
- Module-ը ստանում է host target triple և վերջում անցնում է
  `llvm::verifyModule()`։ Verification-ի ձախողումը նույնպես վերադարձվում է որպես
  `Error`։
- Host `TargetMachine`-ից ստացվում և module-ում պահպանվում է target-ի ճիշտ
  `DataLayout`-ը։
- `RuntimeAbi`-ն սահմանում է `%avium.text` storage type-ը և
  `runtime/include/*.h`-ի բոլոր C function declaration-ները։ `size_t`-ի LLVM
  տիպը վերցվում է `DataLayout`-ից։
- C/LLVM ABI integration test-ը համեմատում է `avium_text`-ի դաշտերի offset-ները,
  չափն ու alignment-ը։
- Բոլոր user subroutine-ները նախապես հայտարարվում են
  `avium.subroutine.<SymbolId>` անունով, ապա գեներացվում են մարմինները։ Այդ
  հերթականությունն արդեն հիմք է forward և recursive կանչերի համար։
- Ստորագրությունների սկզբնական mapping-ը պատրաստ է՝ `REAL → double`,
  `BOOL → i1`, `TEXT/array → ptr`, procedure → `void`, իսկ `TEXT` վերադարձնող
  ֆունկցիայի result storage-ը առաջին `ptr` պարամետրն է։
- Semantic model-ի entry point-ից գեներացվում է C ABI-ի `i32 @main()` wrapper-ը։
- Statement/expression dispatch-ը պատրաստ է։ Դեռ չիրականացված AST հանգույցը
  վերադարձնում է `Error`, ոչ թե կանչում `llvm::report_fatal_error()`։
- Թեստերը ծածկում են դատարկ `Main`-ը, մի քանի ենթածրագրի նախնական հայտարարումը,
  C entry point-ը, module verification-ը և unsupported հանգույցի error path-ը։
- Հաջող pipeline-ի դեպքում CLI-ն LLVM IR-ը տպում է `stdout`, իսկ codegen-ի սխալի
  դեպքում հաղորդագրությունը՝ `stderr` և ավարտվում է `EXIT_FAILURE`-ով։

Դեռ իրականացված չեն փոփոխականների storage-ը, արտահայտությունները,
statement-ների իրական lowering-ը և cleanup-ը։ `book/ch07-llvm-ir.md`-ը լրացվում
է implementation-ին զուգահեռ։

## Անփոփոխ նախագծային պայմանագրեր

### Semantic analysis-ի սահմանը

- Codegen-ը կանչվում է միայն հաջող semantic analysis-ից հետո՝ նույն
  `SymbolTable`-ով և `SemanticModel`-ով։
- Անունների լուծումը, տիպերի ստուգումը, `Main`-ի գոյությունն ու ստորագրությունների
  վավերությունը semantic analyzer-ի պատասխանատվությունն են և codegen-ում չեն
  կրկնվում։
- AST node-երը, variable storage-ները, callee-ները և entry point-ը ընտրվում են
  `SymbolId`-ով, ոչ անունով lookup անելով։
- Հաջող analysis-ից հետո օգտագործվող ամեն expression ունի կոնկրետ տիպ։ Անհնար
  enum ճյուղերը կարող են ավարտվել `std::unreachable()`-ով։
- Array/scalar և function/procedure տարբերակումները semantic ստուգումներ չեն․
  դրանք ընտրում են արդեն վավեր հանգույցի LLVM ներկայացումը։

### Codegen-ի սխալները

- Չիրականացված lowering-ը, անհայտ ներքին function-ը և verifier-ի ձախողումը
  վերադարձվում են `std::unexpected(Error{line, message})`-ով։
- Codegen-ը չի օգտագործում `llvm::report_fatal_error()` և չի ավարտում ամբողջ
  process-ը։
- Վերադարձվող `Error`-ը compiler-ի pipeline-ի սխալն է, ոչ Կեռասի նոր semantic
  diagnostic։

### Կեռասի ենթածրագրերի ABI-ն

- `REAL` պարամետրն ու վերադարձվող արժեքը LLVM `double` են։
- `BOOL` պարամետրն ու վերադարձվող արժեքը LLVM `i1` են։
- `TEXT` պարամետրը storage-ի `ptr` է։ Callee-ն entry block-ում
  `avium_text_copy`-ով ստեղծում է անկախ տեղային պատճեն՝ պահպանելով value
  semantics-ը։
- `TEXT` վերադարձնող ֆունկցիան LLVM-ում վերադարձնում է `void` և առաջին
  պարամետրով ստանում caller-ի հատկացրած `avium_text* result` storage-ը։ Սա
  Կեռասի բացահայտ ABI-ն է, ոչ LLVM `sret`։
- Array parameter-ը borrowed `avium_array*` է և callee-ում չի ոչնչացվում։

### Runtime ABI-ն և LLVM տիպերը

Runtime հայտարարությունները ճշգրտորեն կրկնում են `runtime/include/*.h`-ի C ABI-ն։
`avium_text`-ը երբեք արժեքով չի փոխանցվում կամ վերադարձվում․ input text-երը
pointer-ներ են, իսկ նոր text ստեղծող գործողությունները առաջին
`avium_text* result` պարամետրով գրում են արդյունքը։

| Կեռաս | LLVM հաշվարկային տիպ | Storage/runtime |
| --- | --- | --- |
| `REAL` | `double` | `double` |
| `BOOL` | `i1` | C ABI սահմանին՝ `bool`-ին համապատասխան |
| `TEXT` | հասցեով կառավարվող `%avium.text = type { ptr, iN, i8 }` | `avium_text` |
| զանգված | `ptr` | opaque `avium_array*` |
| procedure | `void` | `void` |
| `REAL`/`BOOL` function | համապատասխան scalar | անմիջական return value |
| `TEXT` function | `void` | առաջին `ptr result` պարամետր |

`%avium.text`-ը named, ոչ packed կառուցվածք է։ Դաշտերն են `data`, `length` և
`owned`, իսկ `iN`-ը target-ի C ABI-ի `size_t` լայնությունն ունի։ Struct layout-ը,
չափը և alignment-ը որոշվում են `DataLayout`-ով և հաստատվում C/LLVM ABI
integration test-ով։ Դաշտերը հասցեագրվում են typed GEP-ով։

### Ownership և lifecycle

- Text literal-ը borrowed է։ `Input`, `STR`, concatenation և text-returning
  function-ը owned արժեք են ստեղծում։
- Borrowed text-ը պահելիս օգտագործվում է `avium_text_copy`, owned temporary-ն
  փոխանցելիս՝ `avium_text_move_assign`։
- Ամեն owned local կամ temporary ոչնչացվում է ճիշտ մեկ անգամ։ Բոլոր `RETURN`-ները
  անցնում են ընդհանուր cleanup epilogue-ով։
- Array slot-ը ստեղծվում է entry block-ում, բայց allocation-ը կատարվում է հենց
  `DIM`-ի կատարման պահին։ Կրկնվող `DIM`-ը նախ ոչնչացնում է հին array-ը։
- Local array-ը ոչնչացվում է ենթածրագրից դուրս գալու ժամանակ, borrowed array
  parameter-ը՝ ոչ։

### Builtin-ների ինքնությունը

Semantic analyzer-ը ճանաչում է `Print`, `Input`, `NUM`, `SQR`, `STR` և `LEN`
builtin-ները, ներառյալ `STR(BOOL)` և `LEN(TEXT|array)` տարբերակները։ Անունները
վերապահված են, իսկ կանչը `SymbolId`-ով կապվում է `SubroutineSymbol`-ին։ Այդ
symbol-ի `builtin` դրոշն ու canonical անունը բավարար են lowering-ի ընտրության
համար, ուստի նույն տեղեկությունը կրկնող առանձին enum չի պահվում։

## Իրականացման փուլերը

### Փուլ 1․ target layout և `RuntimeAbi` (ավարտված)

- Module-ի target triple-ի հետ սահմանել ճիշտ data layout-ը։
- Իրականացնել `RuntimeAbi`-ն՝ `%avium.text` type-ով և բոլոր
  `runtime/include/*.h` function declaration-ներով։
- `DataLayout`-ից ստանալ `size_t`-ի LLVM type-ը և `%avium.text`-ի layout-ը։
- Ավելացնել C/LLVM ABI integration test՝ field offset-ների, struct size-ի և
  alignment-ի համար։
- Թեստերով ամրագրել builtin անունների վերապահված լինելը և signature-ները։

### Փուլ 2․ subroutine frame և storage

- Պահել `SymbolId → llvm::Function*` և ընթացիկ ենթածրագրի
  `SymbolId → Storage` քարտեզները։
- Entry block-ում ստեղծել parameter, local և `FOR` variable slot-երը։
- Սկզբնարժեքավորել storage-ը՝ `FALSE`, `0.0`, դատարկ text և `null` array։
- `REAL`/`BOOL` parameter-ները պահել արժեքով, `TEXT` parameter-ը պատճենել, array
  parameter-ը պահել borrowed reference-ով։
- Ստեղծել return storage և ընդհանուր cleanup block։ Scalar function-ը cleanup-ից
  հետո կատարում է `ret value`, procedure-ն ու `TEXT` function-ը՝ `ret void`։
- Թեստավորել procedure, scalar/text/array parameter-ները, return type-երը,
  forward declaration-ը և recursion-ը։

### Փուլ 3․ scalar արտահայտություններ և `LET`

- Գեներացնել `BOOL`, `REAL` և text literal-ները։ Text literal-ի բայթերը պահել
  private constant storage-ում և կազմել borrowed `%avium.text` handle։
- Իրականացնել scalar variable load-ը և scalar `LET` store-ը։
- Իրականացնել unary `+`, `-`, `NOT` և թվաբանական գործողությունները։ `/`-ը
  floating-point բաժանում է, `\`-ը՝ դեպի զրո կլորացված քանորդ, `MOD`-ը՝
  մնացորդ, `^`-ը՝ `pow` կանչ։
- Իրականացնել numeric և boolean equality/comparison-ները։
- Առանձին թեստերով ամրագրել `\`, `MOD`, `^`, `NaN`, infinity և signed zero
  վարքը։

### Փուլ 4․ control flow

- `IF`/`ELSEIF`/`ELSE`-ի համար կառուցել condition, branch և merge block-եր։
- `WHILE`-ի համար կառուցել condition, body և exit block-եր։
- `AND` և `OR` գործողությունները գեներացնել short-circuit branch-երով ու PHI
  արժեքով։
- `FOR`-ի begin, end և step expression-ները հաշվարկել մեկ անգամ։ Դրական քայլի
  դեպքում կիրառել `<=`, բացասականի դեպքում՝ `>=`։
- Ճիշտ մշակել արդեն terminator ունեցող block-երը և early return-ը։
- Թեստավորել nested control flow-ը, դատարկ branch-երը և loop body-ները։

### Փուլ 5․ user subroutine-ների կանչեր

- `CALL`-ը գեներացնել որպես procedure call։ `REAL`/`BOOL` վերադարձնող `Apply`-ն
  օգտագործում է call-ի անմիջական արդյունքը։
- `TEXT` վերադարձնող `Apply`-ի համար caller-ը հատկացնում է դատարկ temporary
  storage և փոխանցում այն որպես առաջին result pointer։
- `REAL`/`BOOL` argument-ները փոխանցել արժեքով, `TEXT` argument-ները՝ storage-ի
  հասցեով, array-ները՝ borrowed descriptor pointer-ով։
- Callee-ն ընտրել `SemanticModel`-ի `SymbolId`-ով։
- Թեստավորել recursive/mutual կանչերը, մի քանի argument-ը և function result-ի
  օգտագործումը մեծ expression-ում։

### Փուլ 6․ runtime, builtin-ներ և `TEXT`

- Text `&`, `=`, `<>`, `<`, `<=`, `>` և `>=` գործողությունները իջեցնել runtime
  կանչերի։ Համեմատությունը բովանդակությամբ է, ոչ pointer-ով։
- Լուծված builtin symbol-ով իջեցնել `Print`, `Input`, `NUM`, `SQR`, `STR` և
  `LEN(TEXT)` builtin-ները՝ անհրաժեշտ runtime կանչերին source line փոխանցելով։
- Իրականացնել text `LET`, argument, return և temporary ownership-ի copy/move
  կանոնները։
- Cleanup-ում ոչնչացնել բոլոր owned text local-ներն ու temporary-ները։
- Թեստավորել overwrite/self-assignment-ը, owned/borrowed argument-ները, text
  return-ը և բոլոր control-flow ուղիների cleanup-ը։

### Փուլ 7․ զանգվածներ

- `DIM`-ի size expression-ը հաշվարկել մեկ անգամ, հին descriptor-ը ոչնչացնել և
  կանչել `avium_array_create`-ը element tag-ով ու source line-ով։
- Չափի ամբողջ, դրական ու ներկայացնելի լինելը, allocation-ը և element-ների
  սկզբնարժեքավորումը թողնել runtime-ին։
- `a[i]`-ի հասցեն ստանալ համապատասխան typed accessor-ով։ Runtime-ն է ստուգում
  descriptor-ը, element type-ը, index-ի ամբողջ լինելն ու սահմանները։
- Իրականացնել array element load/store-ը և `LEN(array)`-ը։
- Cleanup-ում ոչնչացնել local array-ները, բայց ոչ borrowed parameter-ները։
- Թեստավորել dynamic ու կրկնվող `DIM`-ը, բոլոր element type-երը, parameter-ով
  փոխանցումը և size/index runtime սխալները։

### Փուլ 8․ CLI և ամբողջական ինտեգրում

- Ավելացնել հստակ ելքային ռեժիմներ՝

  ```text
  prunus --emit-ast source.bas
  prunus --emit-llvm source.bas
  prunus --emit-llvm -o output.ll source.bas
  ```

- Հետո ավելացնել object file-ի ու executable-ի ստեղծումը և runtime-ի հետ
  կապակցումը։
- `examples/*.bas` ֆայլերը բաժանել expected-success և expected-diagnostic
  խմբերի ու ավելացնել IR smoke tests։
- Ընտրված ծրագրերը գործարկել native executable-ով կամ JIT-ով և ստուգել stdout-ը,
  stderr-ը և exit status-ը։
- Text/array ownership-ի end-to-end դեպքերը գործարկել AddressSanitizer և
  LeakSanitizer-ով։
- Implementation-ին զուգահեռ լրացնել `book/ch07-llvm-ir.md`-ը։

## Թեստավորման կանոնները

- Ամեն գեներացված module անցնում է `llvm::verifyModule()`։
- Unit test-երը ստուգում են function signature-ները, `%avium.text` layout-ը,
  basic block-երը, runtime call-երը, PHI-ները և short-circuit-ը։
- Մեծ LLVM IR snapshot-ների փոխարեն կիրառվում են verifier, կառուցվածքային
  ստուգումներ և փոքր կայուն IR հատվածներ։
- End-to-end շղթան է `.bas → .ll → object → runtime link → execution`։
- Պարտադիր եզրային դեպքերն են early return-ը, text overwrite/self-assignment-ը,
  owned/borrowed text argument-ը, text return-ը, array parameter mutation-ը,
  invalid size/index-ը, positive/negative `FOR STEP`-ը, short-circuit-ի
  չկատարվող աջ կողմը, `NaN`, infinity, signed zero և recursive/forward call-ը։
- Runtime ABI-ի C թեստերը մնում են առանձին և լրացվում են սահմանային արժեքների ու
  error exit-երի դեպքերով։

## Ավարտված լինելու չափանիշները

Պլանը ավարտված է, երբ Կեռասի բոլոր գործողությունները, control-flow
կառուցվածքները, user subroutine-ները, builtin-ները և զանգվածները ստանում են
վավեր LLVM IR, ownership/cleanup-ը ճիշտ է բոլոր կատարման ուղիներում, հաջող
օրինակները հասնում են execution փուլին, runtime սխալներն ունեն կանխատեսելի
վարք, իսկ compiler-ի և runtime-ի ամբողջ CTest փաթեթն անցնում է։
