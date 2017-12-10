﻿Public Class Utils
    Public Shared Iterator Function FindChildren(Of T As DependencyObject)(source As DependencyObject) As IEnumerable(Of T)
        For Each Child In GetChildObjects(source)
            If Child IsNot Nothing AndAlso TypeOf Child Is T Then
                Yield Child
            End If

            For Each Descendant In FindChildren(Of T)(Child)
                Yield Descendant
            Next
        Next
    End Function

    Public Shared Iterator Function GetChildObjects(Parent As DependencyObject) As IEnumerable(Of DependencyObject)
        If Parent Is Nothing Then Exit Function

        Dim HasValue As Boolean = False
        If TypeOf Parent Is ContentElement Or TypeOf Parent Is FrameworkElement Then
            For Each Obj In LogicalTreeHelper.GetChildren(Parent)
                Dim DepObj As DependencyObject = TryCast(Obj, DependencyObject)
                If DepObj IsNot Nothing Then
                    HasValue = True
                    Yield DepObj
                End If

            Next
        End If
        If Not HasValue Then
            Try
                If Not TypeOf Parent Is RowDefinition And Not TypeOf Parent Is ColumnDefinition Then
                    Dim Count = VisualTreeHelper.GetChildrenCount(Parent)

                    For i = 0 To Count - 1
                        Yield VisualTreeHelper.GetChild(Parent, i)
                    Next
                End If
            Catch e As Exception
            End Try
        End If
    End Function

    Public Shared Function FindVisualParent(Of T As DependencyObject)(Child As DependencyObject) As T
        Dim ParentObj = VisualTreeHelper.GetParent(Child)

        If ParentObj Is Nothing Then Return Nothing

        Dim Parent As T = TryCast(ParentObj, T)
        If Parent IsNot Nothing Then
            Return Parent
        Else
            Return FindVisualParent(Of T)(ParentObj)
        End If
    End Function
End Class

Public MustInherit Class OneWayConverter(Of InputType, TargetType)
    Implements IValueConverter

    Public MustOverride Function Convert(Value As InputType, Parameter As Object) As TargetType

    Private Function Convert(value As Object, targetType As Type, parameter As Object, culture As Globalization.CultureInfo) As Object Implements IValueConverter.Convert
        Return Convert(value, parameter)
    End Function

    Private Function ConvertBack(value As Object, targetType As Type, parameter As Object, culture As Globalization.CultureInfo) As Object Implements IValueConverter.ConvertBack
        Return Nothing
    End Function
End Class

Public MustInherit Class OneWayMultiValueConverter
    Implements IMultiValueConverter

    Public MustOverride Function Convert(values() As Object, targetType As Type, parameter As Object, culture As Globalization.CultureInfo) As Object Implements IMultiValueConverter.Convert

    Public Function ConvertBack(value As Object, targetTypes() As Type, parameter As Object, culture As Globalization.CultureInfo) As Object() Implements IMultiValueConverter.ConvertBack
        Return Nothing
    End Function
End Class

Public MustInherit Class OneWayTwoValueConverter(Of InputType1, InputType2, TargetType)
    Implements IMultiValueConverter

    Public MustOverride Function Convert(Value1 As InputType1, Value2 As InputType2, Parameter As Object) As TargetType

    Public Function Convert1(values() As Object, targetType As Type, parameter As Object, culture As Globalization.CultureInfo) As Object Implements IMultiValueConverter.Convert
        Return Convert(values(0), values(1), parameter)
    End Function

    Public Function ConvertBack(value As Object, targetTypes() As Type, parameter As Object, culture As Globalization.CultureInfo) As Object() Implements IMultiValueConverter.ConvertBack
        Return Nothing
    End Function
End Class
