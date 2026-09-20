Կարդացի `main` ճյուղը։ Ոչ մի ֆայլ չեմ փոփոխել, աշխատանքային ծառը մաքուր է։

Առաջարկվող ընդհանուր հոսքը սա է՝

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

- [main.cxx](/home/armen/Projects/prunus-avium/src/main.cxx:42)-ում ամբողջ frontend pipeline-ն արդեն հավաքված է։ Semantic analysis-ից հետո հիմա արտածվում է միայն AST-ի Lisp տեսքը։
- [SemanticModel](/home/armen/Projects/prunus-avium/src/semantic.hxx:13)-ն արդեն պահում է IR generator-ին անհրաժեշտ հիմնական կապերը՝ `NodeId → SymbolId`, expression type և entry point։
- [SymbolTable](/home/armen/Projects/prunus-avium/src/symbols.hxx:58)-ը տարբերակում է local/parameter/FOR փոփոխականներն ու ենթածրագրերը։
- AST-ն ընդգրկում է բոլոր անհրաժեշտ control-flow հանգույցները, զանգվածները, կանչերն ու վերադարձը։
- [runtime ABI-ի նախագիծը](/home/armen/Projects/prunus-avium/runtime/design.md:25) լավ հիմք է ownership-ի և զանգվածների վարքի համար։
- Top-level [CMakeLists.txt](/home/armen/Projects/prunus-avium/CMakeLists.txt:1)-ը դեռ LLVM-ն ու runtime-ը չի միացնում։
- Համակարգում առկա է LLVM 21.1.8։
- `book/ch07-llvm-ir.md`-ը դեռ դատարկ է, այսինքն implementation-ին զուգահեռ այդ գլխի տեխնիկական բովանդակությունն էլ պետք է ձևավորվի։

## Նախնական պարտադիր որոշումներ

Սրանք արժե փակել մինչև generator գրելն սկսելը։

1. **Կայունացնել runtime ABI-ն։**  
   Ներկա C ABI-ում `avium_text`-ը փոխանցվում է արժեքով։ Clang-ը այս հարթակում այն իջեցնում է `byval` և `sret` ձևերի, հետևաբար LLVM-ում պարզապես `{ ptr, i64, i8 }` տիպով կանչ ստեղծելը ABI-compatible չէ։

   Խորհուրդս՝ aggregate-ների runtime API-ն դարձնել pointer/out-parameter հիմքով, օրինակ՝

   ```c
   void avium_input(avium_text* result, unsigned line);
   void avium_text_concat(
       avium_text* result,
       const avium_text* left,
       const avium_text* right,
       unsigned line);
   ```

   Սա միաժամանակ պարզեցնում է LLVM և ապագա C backend-ները և հարթակից կախված ABI lowering չի պահանջում։

2. **Հստակեցնել builtin-ների ցանկը։**  
   Semantic analyzer-ը հիմա ճանաչում է `Print`, `Input`, `NUM`, `SQR`, մինչդեռ runtime-ը և օրինակները նաև նախատեսում են `STR` ու `LEN`։ Առաջարկում եմ `SubroutineSymbol`-ում `bool builtin`-ի փոխարեն ունենալ `BuiltinKind`, որպեսզի generator-ը builtin-ը ճանաչի semantic identity-ով, ոչ թե անվան տողը համեմատելով։

3. **Սահմանել dynamic `DIM`-ի lifecycle-ը։**  
   Առաջարկվող կանոնը՝ storage-ը ստեղծել ենթածրագրի entry block-ում, բայց զանգվածը հատկացնել հենց `DIM`-ի կատարման պահին։ Եթե նույն `DIM`-ը կրկին կատարվում է ցիկլում, նախ ոչնչացնել հին զանգվածը, ապա ստեղծել նորը։

4. **Սահմանել `TEXT` պարամետրի value semantics-ը։**  
   Պարզ պարամետրերը փոխանցվում են արժեքով, հետևաբար `TEXT` պարամետրը callee-ի մուտքում պետք է դառնա անկախ պատճեն։ Զանգվածային պարամետրերը մնում են borrowed հղումներ։

## LLVM տիպերի մոդելը

| Կեռաս | LLVM հաշվարկային տիպ | Պահպանում/runtime |
|---|---|---|
| `REAL` | `double` | `double` |
| `BOOL` | `i1` | ABI սահմանին ցանկալի է ֆիքսված 8-bit ներկայացում |
| `TEXT` | հասցեով կառավարվող named struct | `avium_text` |
| զանգված | `ptr` | opaque `avium_array*` |
| procedure | `void` | `void` |
| `TEXT` վերադարձ | out/sret slot | ownership-ը փոխանցվում է կանչողին |

`size_t`-ի լայնությունը պետք է վերցնել module-ի `DataLayout`-ից, ոչ թե ամրագրել որպես `i64`։

## Իրականացման փուլերը

1. **Build-ի հիմք**

   - Root project-ը դարձնել `LANGUAGES C CXX`։
   - Միացնել `runtime/`-ը `add_subdirectory(runtime)`-ով։
   - Ավելացնել `find_package(LLVM CONFIG REQUIRED)`։
   - Compiler-ի frontend աղբյուրները հանել առանձին library target-ի մեջ, որպեսզի executable-ն ու թեստերը նույն target-ը օգտագործեն։
   - Սկզբնական փուլում կապել միայն LLVM Core/Support բաղադրիչները։

2. **Generator-ի կմախք**

   Ավելացնել մոտավորապես՝

   ```text
   src/irgenerator.hxx
   src/irgenerator.cxx
   src/runtimeabi.hxx
   src/runtimeabi.cxx
   ```

   `IRGenerator`-ը պետք է ստանա `SymbolTable`, `SemanticModel` և target configuration։ AST-ը չպետք է LLVM-specific տվյալներով փոփոխվի։

   Արդյունքը պետք է լինի `std::unique_ptr<llvm::Module>` կամ սխալ ներկայացնող `llvm::Expected`։

3. **Module և հայտարարությունների առաջին անցում**

   - Սահմանել target triple և data layout։
   - Հայտարարել runtime ֆունկցիաները։
   - Նախապես հայտարարել բոլոր user subroutine-ները՝ forward call և recursion թույլ տալու համար։
   - LLVM անունները կառուցել `SymbolId`-ով կամ անվտանգ mangling-ով։
   - Գեներացնել սովորական C `main`, որը կանչում է semantic model-ում նշված Cherry `Main`-ը և վերադարձնում `0`։

4. **Ֆունկցիաների մարմիններ և storage**

   - Բոլոր local slot-երը ստեղծել entry block-ում։
   - Օգտագործել `SymbolId → Storage` քարտեզ, ոչ թե փոփոխականի անուն։
   - `REAL`/`BOOL` local-ները սկզբնարժեքավորել։
   - `TEXT` local-ները սկսել անվտանգ դատարկ արժեքով։
   - Local array slot-երը սկսել `null`-ով։
   - Parameter-ների համար կիրառել պարզ արժեքի copy և զանգվածի borrowed-reference կանոնները։

5. **Արտահայտություններ**

   Հերթականությունը՝

   - literal և variable load,
   - unary գործողություններ,
   - թվային arithmetic և comparison,
   - տեքստային comparison և concatenation,
   - array indexing runtime-ի typed accessor-ներով,
   - user և builtin function call,
   - `AND`/`OR` short-circuit՝ basic block-երով և `phi`-ով։

   `\`, `MOD`, `^` և floating-point comparison-ների համար պետք է առանձին թեստերով ամրագրել NaN/Infinity վարքը։

6. **Հրամաններ և control flow**

   - `LET`՝ scalar store կամ text copy/move։
   - `DIM`՝ array create։
   - `IF/ELSEIF/ELSE`՝ branch chain և merge block։
   - `WHILE`՝ condition/body/exit blocks։
   - `FOR`՝ begin/end/step-ը մեկ անգամ հաշվարկել, այնուհետև քայլի նշանից կախված `<=` կամ `>=` պայման։
   - `CALL`։
   - `RETURN`՝ արժեքը տեղափոխել return slot, ապա գնալ ընդհանուր cleanup block։

7. **Ownership և cleanup**

   Սա generator-ի ամենազգայուն մասն է։

   - Text literal-ը borrowed է։
   - `Input`, `STR`, concatenation և text-returning function-ը owned արժեք են տալիս։
   - Borrowed RHS-ը variable-ին վերագրելիս նախ `avium_text_copy`։
   - Owned temporary-ն վերագրելիս՝ `avium_text_move_assign`։
   - `TEXT` վերադարձնելիս ownership-ը փոխանցել return slot-ին։
   - Բոլոր `RETURN`-ները տանել մեկ cleanup epilogue։
   - Cleanup-ում ոչնչացնել local text-երն ու local array-ները, բայց ոչ array parameter-ները։
   - Յուրաքանչյուր control-flow ճանապարհով ժամանակավոր owned text-ը պետք է ոչնչացվի ճիշտ մեկ անգամ։

8. **CLI ինտեգրում**

   Սկզբում պահպանել ներկա վարքը և ավելացնել հստակ ռեժիմներ՝

   ```text
   prunus --emit-ast source.bas
   prunus --emit-llvm source.bas
   prunus --emit-llvm -o output.ll source.bas
   ```

   Object file և executable ստեղծելը թողնել հաջորդ փուլին՝ `--compile` կամ `-c` ռեժիմով։

## Թեստավորման ռազմավարություն

- `tests/irgenerator_test.cxx`՝ փոքր AST/program → LLVM module։
- Ամեն գեներացված module-ի համար պարտադիր `llvm::verifyModule`։
- IR կառուցվածքային թեստեր՝ function signature, basic block, runtime call, short-circuit։
- End-to-end թեստեր՝ `.bas → .ll → object → runtime link → execution`։
- Առանձին դեպքեր՝
  - early return և cleanup,
  - text overwrite/self-assignment,
  - owned/borrowed text argument,
  - text վերադարձ,
  - array parameter-ի փոփոխության տեսանելիություն,
  - invalid size/index runtime error,
  - դրական և բացասական `FOR STEP`,
  - short-circuit-ի աջ կողմի չկատարում,
  - `NaN`, infinity և signed zero,
  - recursive և forward function call։
- AddressSanitizer/LeakSanitizer end-to-end թեստեր՝ text/array ownership-ի համար։

## Առաջարկվող PR-ների բաժանումը

1. ABI-ի հստակեցում, builtin registry և runtime compatibility թեստեր։
2. LLVM/CMake skeleton, type mapping, scalar expressions ու subroutine-ներ։
3. Control flow և scalar builtins։
4. `TEXT` ownership և text builtins։
5. Զանգվածներ ու `LEN`։
6. CLI, object generation, runtime linking և end-to-end թեստեր։
7. `book/ch07-llvm-ir.md` փաստաթղթավորում։

Այս կառուցվածքով ամենաբարդ մասը՝ ABI-ն ու ownership-ը, լուծվում է սկզբում, իսկ հետագա IR generation-ը դառնում է հիմնականում ուղիղ AST→CFG աշխատանք։
