; ModuleID = 'bf'
source_filename = "bf"

declare void @native_putchar({}*, i32)

declare i32 @native_getchar({}*)

define i8* @code({}* %0, i8* noalias %1) {
l1:
  %2 = alloca i8*
  store i8* %1, i8** %2
  %3 = load i8*, i8** %2
  %4 = load i8, i8* %3
  %5 = add i8 %4, 8
  store i8 %5, i8* %3
  br label %l2

l2:                                               ; preds = %l10, %l1
  %6 = phi i8* [ %3, %l1 ], [ %103, %l10 ]
  %7 = load i8, i8* %6
  %8 = icmp ne i8 %7, 0
  br i1 %8, label %l3, label %l4

l3:                                               ; preds = %l2
  %9 = phi i8* [ %6, %l2 ]
  %10 = getelementptr i8, i8* %9, i64 1
  %11 = load i8, i8* %10
  %12 = add i8 %11, 4
  store i8 %12, i8* %10
  br label %l5

l4:                                               ; preds = %l2
  %13 = phi i8* [ %6, %l2 ]
  %14 = getelementptr i8, i8* %13, i64 2
  %15 = load i8, i8* %14
  %16 = zext i8 %15 to i32
  call void @native_putchar({}* %0, i32 %16)
  %17 = getelementptr i8, i8* %14, i64 2
  %18 = load i8, i8* %17
  %19 = sub i8 %18, 3
  store i8 %19, i8* %17
  %20 = load i8, i8* %17
  %21 = zext i8 %20 to i32
  call void @native_putchar({}* %0, i32 %21)
  %22 = load i8, i8* %17
  %23 = add i8 %22, 7
  store i8 %23, i8* %17
  %24 = load i8, i8* %17
  %25 = zext i8 %24 to i32
  call void @native_putchar({}* %0, i32 %25)
  %26 = load i8, i8* %17
  %27 = zext i8 %26 to i32
  call void @native_putchar({}* %0, i32 %27)
  %28 = load i8, i8* %17
  %29 = add i8 %28, 3
  store i8 %29, i8* %17
  %30 = load i8, i8* %17
  %31 = zext i8 %30 to i32
  call void @native_putchar({}* %0, i32 %31)
  %32 = getelementptr i8, i8* %17, i64 1
  %33 = load i8, i8* %32
  %34 = zext i8 %33 to i32
  call void @native_putchar({}* %0, i32 %34)
  %35 = getelementptr i8, i8* %32, i64 -2
  %36 = load i8, i8* %35
  %37 = sub i8 %36, 1
  store i8 %37, i8* %35
  %38 = load i8, i8* %35
  %39 = zext i8 %38 to i32
  call void @native_putchar({}* %0, i32 %39)
  %40 = getelementptr i8, i8* %35, i64 1
  %41 = load i8, i8* %40
  %42 = zext i8 %41 to i32
  call void @native_putchar({}* %0, i32 %42)
  %43 = load i8, i8* %40
  %44 = add i8 %43, 3
  store i8 %44, i8* %40
  %45 = load i8, i8* %40
  %46 = zext i8 %45 to i32
  call void @native_putchar({}* %0, i32 %46)
  %47 = load i8, i8* %40
  %48 = sub i8 %47, 6
  store i8 %48, i8* %40
  %49 = load i8, i8* %40
  %50 = zext i8 %49 to i32
  call void @native_putchar({}* %0, i32 %50)
  %51 = load i8, i8* %40
  %52 = sub i8 %51, 8
  store i8 %52, i8* %40
  %53 = load i8, i8* %40
  %54 = zext i8 %53 to i32
  call void @native_putchar({}* %0, i32 %54)
  %55 = getelementptr i8, i8* %40, i64 1
  %56 = load i8, i8* %55
  %57 = add i8 %56, 1
  store i8 %57, i8* %55
  %58 = load i8, i8* %55
  %59 = zext i8 %58 to i32
  call void @native_putchar({}* %0, i32 %59)
  %60 = getelementptr i8, i8* %55, i64 1
  %61 = load i8, i8* %60
  %62 = add i8 %61, 2
  store i8 %62, i8* %60
  %63 = load i8, i8* %60
  %64 = zext i8 %63 to i32
  call void @native_putchar({}* %0, i32 %64)
  ret i8* %60

l5:                                               ; preds = %l6, %l3
  %65 = phi i8* [ %10, %l3 ], [ %81, %l6 ]
  %66 = load i8, i8* %65
  %67 = icmp ne i8 %66, 0
  br i1 %67, label %l6, label %l7

l6:                                               ; preds = %l5
  %68 = phi i8* [ %65, %l5 ]
  %69 = getelementptr i8, i8* %68, i64 1
  %70 = load i8, i8* %69
  %71 = add i8 %70, 2
  store i8 %71, i8* %69
  %72 = getelementptr i8, i8* %69, i64 1
  %73 = load i8, i8* %72
  %74 = add i8 %73, 3
  store i8 %74, i8* %72
  %75 = getelementptr i8, i8* %72, i64 1
  %76 = load i8, i8* %75
  %77 = add i8 %76, 3
  store i8 %77, i8* %75
  %78 = getelementptr i8, i8* %75, i64 1
  %79 = load i8, i8* %78
  %80 = add i8 %79, 1
  store i8 %80, i8* %78
  %81 = getelementptr i8, i8* %78, i64 -4
  %82 = load i8, i8* %81
  %83 = sub i8 %82, 1
  store i8 %83, i8* %81
  br label %l5

l7:                                               ; preds = %l5
  %84 = phi i8* [ %65, %l5 ]
  %85 = getelementptr i8, i8* %84, i64 1
  %86 = load i8, i8* %85
  %87 = add i8 %86, 1
  store i8 %87, i8* %85
  %88 = getelementptr i8, i8* %85, i64 1
  %89 = load i8, i8* %88
  %90 = sub i8 %89, 1
  store i8 %90, i8* %88
  %91 = getelementptr i8, i8* %88, i64 1
  %92 = load i8, i8* %91
  %93 = add i8 %92, 1
  store i8 %93, i8* %91
  %94 = getelementptr i8, i8* %91, i64 2
  %95 = load i8, i8* %94
  %96 = add i8 %95, 1
  store i8 %96, i8* %94
  br label %l8

l8:                                               ; preds = %l9, %l7
  %97 = phi i8* [ %94, %l7 ], [ %101, %l9 ]
  %98 = load i8, i8* %97
  %99 = icmp ne i8 %98, 0
  br i1 %99, label %l9, label %l10

l9:                                               ; preds = %l8
  %100 = phi i8* [ %97, %l8 ]
  %101 = getelementptr i8, i8* %100, i64 -1
  br label %l8

l10:                                              ; preds = %l8
  %102 = phi i8* [ %97, %l8 ]
  %103 = getelementptr i8, i8* %102, i64 -1
  %104 = load i8, i8* %103
  %105 = sub i8 %104, 1
  store i8 %105, i8* %103
  br label %l2
}
