; ModuleID = 'bf'
source_filename = "bf"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @native_putchar({}*, i32) local_unnamed_addr

define i8* @code({}* %0, i8* noalias %1) local_unnamed_addr {
l1:
  %2 = load i8, i8* %1, align 1
  %3 = add i8 %2, 8
  store i8 %3, i8* %1, align 1
  %4 = icmp eq i8 %3, 0
  br i1 %4, label %l4, label %l3

l3:                                               ; preds = %l1, %l10
  %5 = phi i8* [ %76, %l10 ], [ %1, %l1 ]
  %6 = getelementptr i8, i8* %5, i64 1
  %7 = load i8, i8* %6, align 1
  %8 = add i8 %7, 4
  store i8 %8, i8* %6, align 1
  %9 = icmp eq i8 %8, 0
  %10 = getelementptr i8, i8* %5, i64 2
  %11 = load i8, i8* %10, align 1
  %.phi.trans.insert = getelementptr i8, i8* %5, i64 3
  br i1 %9, label %l3.l7_crit_edge, label %l6.lr.ph

l3.l7_crit_edge:                                  ; preds = %l3
  %.pre = load i8, i8* %.phi.trans.insert, align 1
  %.phi.trans.insert23 = getelementptr i8, i8* %5, i64 4
  %.pre24 = load i8, i8* %.phi.trans.insert23, align 1
  br label %l7

l6.lr.ph:                                         ; preds = %l3
  %12 = getelementptr i8, i8* %5, i64 4
  %13 = getelementptr i8, i8* %5, i64 5
  %.promoted = load i8, i8* %.phi.trans.insert, align 1
  %.promoted10 = load i8, i8* %12, align 1
  %.promoted12 = load i8, i8* %13, align 1
  %14 = add i8 %11, 8
  %15 = shl i8 %7, 1
  %16 = add i8 %14, %15
  %17 = mul i8 %7, 3
  %18 = add i8 %.promoted10, 12
  %19 = add i8 %.promoted, 12
  %20 = add i8 %19, %17
  %21 = add i8 %18, %17
  %22 = add i8 %8, %.promoted12
  store i8 %16, i8* %10, align 1
  store i8 %20, i8* %.phi.trans.insert, align 1
  store i8 %21, i8* %12, align 1
  store i8 %22, i8* %13, align 1
  store i8 0, i8* %6, align 1
  br label %l7

l4:                                               ; preds = %l10, %l1
  %.lcssa4 = phi i8* [ %1, %l1 ], [ %76, %l10 ]
  %23 = getelementptr i8, i8* %.lcssa4, i64 2
  %24 = load i8, i8* %23, align 1
  %25 = zext i8 %24 to i32
  tail call void @native_putchar({}* %0, i32 %25)
  %26 = getelementptr i8, i8* %.lcssa4, i64 4
  %27 = load i8, i8* %26, align 1
  %28 = add i8 %27, -3
  store i8 %28, i8* %26, align 1
  %29 = zext i8 %28 to i32
  tail call void @native_putchar({}* %0, i32 %29)
  %30 = load i8, i8* %26, align 1
  %31 = add i8 %30, 7
  store i8 %31, i8* %26, align 1
  %32 = zext i8 %31 to i32
  tail call void @native_putchar({}* %0, i32 %32)
  %33 = load i8, i8* %26, align 1
  %34 = zext i8 %33 to i32
  tail call void @native_putchar({}* %0, i32 %34)
  %35 = load i8, i8* %26, align 1
  %36 = add i8 %35, 3
  store i8 %36, i8* %26, align 1
  %37 = zext i8 %36 to i32
  tail call void @native_putchar({}* %0, i32 %37)
  %38 = getelementptr i8, i8* %.lcssa4, i64 5
  %39 = load i8, i8* %38, align 1
  %40 = zext i8 %39 to i32
  tail call void @native_putchar({}* %0, i32 %40)
  %41 = getelementptr i8, i8* %.lcssa4, i64 3
  %42 = load i8, i8* %41, align 1
  %43 = add i8 %42, -1
  store i8 %43, i8* %41, align 1
  %44 = zext i8 %43 to i32
  tail call void @native_putchar({}* %0, i32 %44)
  %45 = load i8, i8* %26, align 1
  %46 = zext i8 %45 to i32
  tail call void @native_putchar({}* %0, i32 %46)
  %47 = load i8, i8* %26, align 1
  %48 = add i8 %47, 3
  store i8 %48, i8* %26, align 1
  %49 = zext i8 %48 to i32
  tail call void @native_putchar({}* %0, i32 %49)
  %50 = load i8, i8* %26, align 1
  %51 = add i8 %50, -6
  store i8 %51, i8* %26, align 1
  %52 = zext i8 %51 to i32
  tail call void @native_putchar({}* %0, i32 %52)
  %53 = load i8, i8* %26, align 1
  %54 = add i8 %53, -8
  store i8 %54, i8* %26, align 1
  %55 = zext i8 %54 to i32
  tail call void @native_putchar({}* %0, i32 %55)
  %56 = load i8, i8* %38, align 1
  %57 = add i8 %56, 1
  store i8 %57, i8* %38, align 1
  %58 = zext i8 %57 to i32
  tail call void @native_putchar({}* %0, i32 %58)
  %59 = getelementptr i8, i8* %.lcssa4, i64 6
  %60 = load i8, i8* %59, align 1
  %61 = add i8 %60, 2
  store i8 %61, i8* %59, align 1
  %62 = zext i8 %61 to i32
  tail call void @native_putchar({}* %0, i32 %62)
  ret i8* %59

l7:                                               ; preds = %l3.l7_crit_edge, %l6.lr.ph
  %63 = phi i8 [ %21, %l6.lr.ph ], [ %.pre24, %l3.l7_crit_edge ]
  %64 = phi i8 [ %20, %l6.lr.ph ], [ %.pre, %l3.l7_crit_edge ]
  %.lcssa = phi i8 [ %16, %l6.lr.ph ], [ %11, %l3.l7_crit_edge ]
  %65 = add i8 %.lcssa, 1
  store i8 %65, i8* %10, align 1
  %66 = getelementptr i8, i8* %5, i64 3
  %67 = add i8 %64, -1
  store i8 %67, i8* %66, align 1
  %68 = getelementptr i8, i8* %5, i64 4
  %69 = add i8 %63, 1
  store i8 %69, i8* %68, align 1
  %70 = getelementptr i8, i8* %5, i64 6
  %71 = load i8, i8* %70, align 1
  %72 = add i8 %71, 1
  store i8 %72, i8* %70, align 1
  br label %l8

l8:                                               ; preds = %l8, %l7
  %73 = phi i8 [ %72, %l7 ], [ %77, %l8 ]
  %74 = phi i8* [ %70, %l7 ], [ %76, %l8 ]
  %75 = icmp eq i8 %73, 0
  %76 = getelementptr i8, i8* %74, i64 -1
  %77 = load i8, i8* %76, align 1
  br i1 %75, label %l10, label %l8

l10:                                              ; preds = %l8
  %78 = add i8 %77, -1
  store i8 %78, i8* %76, align 1
  %79 = icmp eq i8 %78, 0
  br i1 %79, label %l4, label %l3
}
