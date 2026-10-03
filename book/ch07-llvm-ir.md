# LLVM և կոդի գեներացիա

Սեմանտիկ վերլուծությունից հետո Կեռասի ծրագիրը վերածվում է LLVM IR-ի։ Այս
փուլը նորից չի լուծում անունները և չի ստուգում տիպերը․ այն օգտագործում է
`SymbolTable`-ում ու `SemanticModel`-ում արդեն պահպանված կապերը։ Այդ բաժանումը
թույլ է տալիս code generator-ին կենտրոնանալ արժեքների ներկայացման, control
flow-ի և runtime կանչերի վրա։

## Թիրախի նկարագրությունը

LLVM module-ի target triple-ը միայն թիրախի անունն է։ Հիշողության ճշգրիտ
դասավորության համար անհրաժեշտ է նաև `DataLayout`-ը։ Compiler-ը host target-ը
գրանցում է LLVM-ում, ստեղծում է համապատասխան `TargetMachine` և դրանից ստացված
layout-ը պահում module-ում։ Այս քայլը կատարվում է մինչև runtime տիպերի
կառուցումը։

Առաջին backend-ը գեներացնում է նույն թիրախի կոդը, որի համար կառուցվել է
`avium-runtime`-ը։ Հետևաբար runtime-ի C ABI-ն և module-ի LLVM ABI-ն պետք է
համընկնեն։ Օրինակ՝ `size_t`-ի LLVM տիպը ստացվում է `DataLayout`-ի pointer-sized
integer type-ից, այլ ոչ թե հաստատուն `i64` ընտրելով։

## Runtime ABI

`RuntimeAbi` դասը մեկ տեղում նկարագրում է `runtime/include` header-ների բոլոր
արտաքին ֆունկցիաները։ Այն module-ում հայտարարում է տեքստերի, զանգվածների,
մուտք/ելքի և թվային գործողությունների C entry point-ները և դրանք պահում է
պարզ, անունով դաշտերում, օրինակ՝ `textCopy`, `arrayCreate` և `printText`։

Կեռասի `TEXT` storage-ը ներկայացվում է named, ոչ packed կառուցվածքով․

```llvm
%avium.text = type { ptr, iN, i8 }
```

Դաշտերը համապատասխանաբար `data`, `length` և `owned` են։ `iN`-ը target-ի
`size_t`-ի լայնությունն ունի, իսկ `owned`-ը `i8` storage է, որովհետև C-ի
`bool`-ը հիշողության մեջ մեկ բայթ է։ Միևնույն ժամանակ C ֆունկցիայի անմիջական
`bool` պարամետրը LLVM signature-ում `i1 zeroext` է։

ABI integration test-ը C compiler-ով չափում է `avium_text`-ի դաշտերի
offset-ները, ընդհանուր չափը և alignment-ը, ապա համեմատում դրանք LLVM
`StructLayout`-ի արդյունքների հետ։ Այդ թեստը թույլ չի տալիս, որ C կառուցվածքն ու
LLVM ներկայացումը աննկատ հեռանան իրարից։

## Ներդրված ենթածրագրերը

`Print`, `Input`, `NUM`, `SQR`, `STR` և `LEN` անունները վերապահված են և չեն
կարող վերասահմանվել ծրագրում։ Semantic analysis-ը կանչը կապում է
`SubroutineSymbol`-ի հետ, որի `builtin` դրոշը տարբերակում է այն user-defined
ենթածրագրից։ Քանի որ builtin անունները եզակի և անփոփոխ են, առանձին enum պահելու
կարիք չկա։
