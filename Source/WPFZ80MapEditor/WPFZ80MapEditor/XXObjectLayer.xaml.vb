Imports System.Collections.ObjectModel
Imports System.Windows.Controls.Primitives
Imports System.Windows.Media.Effects

Public Class XXObjectLayer
    Inherits MapLayer

    Public Property ObjectCollection As ObservableCollection(Of IBaseGeneralObject)
        Get
            Return GetValue(ObjectCollectionProperty)
        End Get

        Set(ByVal value As ObservableCollection(Of IBaseGeneralObject))
            SetValue(ObjectCollectionProperty, value)
        End Set
    End Property

    Public Shared ReadOnly ObjectCollectionProperty As DependencyProperty = _
                           DependencyProperty.Register("ObjectCollection", _
                           GetType(ObservableCollection(Of IBaseGeneralObject)), GetType(XXObjectLayer), _
                           New PropertyMetadata(Nothing))

    Public Property ObjectType As Type
        Get
            Return GetValue(ObjectTypeProperty)
        End Get

        Set(ByVal value As Type)
            SetValue(ObjectTypeProperty, value)
        End Set
    End Property

    Public Shared ReadOnly ObjectTypeProperty As DependencyProperty = _
                           DependencyProperty.Register("ObjectType", _
                           GetType(Type), GetType(XXObjectLayer), _
                           New PropertyMetadata(Nothing))

    Public Property ObjectEffect As Effect
        Get
            Return GetValue(ObjectEffectProperty)
        End Get

        Set(ByVal value As Effect)
            SetValue(ObjectEffectProperty, value)
        End Set
    End Property

    Public Shared ReadOnly ObjectEffectProperty As DependencyProperty = _
                           DependencyProperty.Register("ObjectEffect", _
                           GetType(Effect), GetType(XXObjectLayer), _
                           New PropertyMetadata(Nothing))

    Public Property ObjectsUnderClick As Collection(Of IBaseGeneralObject)
        Get
            Return GetValue(ObjectsUnderClickProperty)
        End Get

        Set(ByVal value As Collection(Of IBaseGeneralObject))
            SetValue(ObjectsUnderClickProperty, value)
        End Set
    End Property

    Public Shared ReadOnly ObjectsUnderClickProperty As DependencyProperty = _
                           DependencyProperty.Register("ObjectsUnderClick", _
                           GetType(Collection(Of IBaseGeneralObject)), GetType(XXObjectLayer), _
                           New PropertyMetadata(Nothing))


#Region "Canvas events"
    Private _StartDrag As New Point

    Private Sub ObjectCanvas_MouseLeftButtonDown(sender As System.Object, e As System.Windows.Input.MouseButtonEventArgs) Handles ObjectCanvas.MouseLeftButtonDown
        SelectionRect.Width = 0
        SelectionRect.Height = 0

        Dim SelRect As New Rect
        Dim CurPoint = Mouse.GetPosition(ObjectCanvas)
        CurPoint.X = Math.Max(0, Math.Min(256, CurPoint.X))
        CurPoint.Y = Math.Max(0, Math.Min(256, CurPoint.Y))
        SelRect.X = Math.Max(0, Math.Min(256, CurPoint.X))
        SelRect.Y = Math.Max(0, Math.Min(256, CurPoint.Y))
        SelRect.Width = 1
        SelRect.Height = 1

        For Each ZObj As IBaseGeneralObject In ObjectListBox.ItemsSource
            Dim IntersectResult As Rect = Rect.Intersect(ZObj.Bounds, SelRect)
            If Not IntersectResult.IsEmpty() Then
                Exit Sub
            End If
        Next

        _StartDrag = Mouse.GetPosition(ObjectCanvas)

        If sender.CaptureMouse() Then
            SelectionRect.Visibility = Windows.Visibility.Visible
            'DeselectAll()
            ObjectListBox.Focus()

            e.Handled = True
        End If
    End Sub

    Private Sub ObjectCanvas_MouseLeftButtonUp(sender As System.Object, e As System.Windows.Input.MouseButtonEventArgs) Handles ObjectCanvas.MouseLeftButtonUp
        If Mouse.Captured Is sender Then
            sender.ReleaseMouseCapture()
            SelectionRect.Visibility = Windows.Visibility.Hidden

            Dim SelRect As New Rect
            SelRect.X = Canvas.GetLeft(SelectionRect)
            SelRect.Y = Canvas.GetTop(SelectionRect)
            SelRect.Width = SelectionRect.Width
            SelRect.Height = SelectionRect.Height

            SelectObjectsInRect(SelRect)

            e.Handled = True
        End If
    End Sub

    Private Sub ObjectCanvas_MouseMove(sender As System.Object, e As System.Windows.Input.MouseEventArgs) Handles ObjectCanvas.MouseMove
        If Mouse.Captured Is sender Then
            Dim CurPoint = Mouse.GetPosition(ObjectCanvas)
            CurPoint.X = Math.Max(0, Math.Min(256, CurPoint.X))
            CurPoint.Y = Math.Max(0, Math.Min(256, CurPoint.Y))

            Canvas.SetLeft(SelectionRect, Math.Min(CurPoint.X, _StartDrag.X))
            Canvas.SetTop(SelectionRect, Math.Min(CurPoint.Y, _StartDrag.Y))
            SelectionRect.Width = Math.Abs(CurPoint.X - _StartDrag.X)
            SelectionRect.Height = Math.Abs(CurPoint.Y - _StartDrag.Y)

            Dim SelRect As New Rect
            SelRect.X = Canvas.GetLeft(SelectionRect)
            SelRect.Y = Canvas.GetTop(SelectionRect)
            SelRect.Width = SelectionRect.Width
            SelRect.Height = SelectionRect.Height

            SelectObjectsInRect(SelRect)

            e.Handled = True
        End If
    End Sub

    Private Sub SelectObjectsInRect(SelRect As Rect)
        ObjectListBox.SelectedItems.Clear()

        For Each ZObj As IBaseGeneralObject In ObjectListBox.ItemsSource
            Dim IntersectResult As Rect = Rect.Intersect(ZObj.Bounds, SelRect)
            If Not IntersectResult.IsEmpty() Then
                If (IntersectResult.Width * IntersectResult.Height) > (ZObj.Bounds.Width * ZObj.Bounds.Height * 0.75) Then
                    ObjectListBox.SelectedItems.Add(ZObj)
                ElseIf IntersectResult.Equals(SelRect) Then
                    ObjectListBox.SelectedItems.Add(ZObj)
                End If
            End If
        Next
    End Sub
#End Region

    Private _IsDraggingObject As Boolean = False
    Private _StartObjectDrag As New Point
    Private _StartClickTime As Long? = Nothing
    Private _FirstClick As Boolean = True
    Private _LastItemClicked As IBaseGeneralObject = Nothing

    Private Sub RaiseCoordinatesUpdated()
        If ObjectListBox.SelectedItems.Count > 0 Then

            Dim Args = New CoordinatesUpdatedArgs(MapLayer.CoordinatesUpdatedEvent, New Point(ObjectListBox.SelectedItems(0).X, ObjectListBox.SelectedItems(0).Y))
            MyBase.RaiseEvent(Args)

        End If
    End Sub

    Public Sub ItemContainer_MouseLeftButtonDown(sender As System.Object, e As System.Windows.Input.MouseButtonEventArgs)
        If _LastItemClicked IsNot sender.DataContext Then
            _FirstClick = True
        End If
        If _StartClickTime IsNot Nothing AndAlso Environment.TickCount - _StartClickTime > 800 Then
            _FirstClick = True
        End If
        If _FirstClick OrElse _StartClickTime Is Nothing Then
            _StartClickTime = Environment.TickCount
        End If

        If Not _FirstClick And ((Environment.TickCount - _StartClickTime) < 400) Then
            ApplicationCommands.Properties.Execute(Nothing, Me)
            Exit Sub
        End If

        _StartObjectDrag = Mouse.GetPosition(ObjectCanvas)
        If sender.CaptureMouse() Then
            e.Handled = True
            UndoManager.PushUndoState(Map, UndoManager.TypeFlagFromType(ObjectType))

            RaiseCoordinatesUpdated()
            ObjectListBox.Focus()
        End If

        _LastItemClicked = sender.DataContext
    End Sub

    Public Sub ItemContainer_MouseDoubleClick(sender As System.Object, e As System.Windows.Input.MouseButtonEventArgs)
    End Sub

    Public Sub ItemContainer_MouseMove(sender As System.Object, e As System.Windows.Input.MouseEventArgs)

        Dim Pos = Mouse.GetPosition(ObjectCanvas)
        If Not _IsDraggingObject And e.LeftButton = MouseButtonState.Pressed Then
            If Math.Abs(Pos.X - _StartObjectDrag.X) > SystemParameters.MinimumHorizontalDragDistance OrElse
               Math.Abs(Pos.Y - _StartObjectDrag.Y) > SystemParameters.MinimumVerticalDragDistance Then

                _IsDraggingObject = True
                If Not ObjectListBox.SelectedItems.Contains(sender.Content) Then
                    ObjectListBox.SelectedItem = sender.Content
                End If

                For Each ZObj As IBaseGeneralObject In ObjectListBox.SelectedItems
                    ZObj.PreviousVersion = ZObj.Clone
                Next
            End If
        End If

        If _IsDraggingObject Then
            Dim CurPoint = Mouse.GetPosition(ObjectCanvas)
            Dim Diff = CurPoint - _StartObjectDrag

            For Each ZObj As IBaseGeneralObject In ObjectListBox.SelectedItems
                If ZObj.PreviousVersion IsNot Nothing Then
                    Dim StartX = SPASMHelper.Eval(ZObj.PreviousVersion.Args(0).Value)
                    Dim StartY = SPASMHelper.Eval(ZObj.PreviousVersion.Args(1).Value)

                    ZObj.Args(0).Value = CInt(Math.Round(StartX + Diff.X))
                    ZObj.Args(1).Value = CInt(Math.Round(StartY + Diff.Y))
                End If
            Next

            RaiseCoordinatesUpdated()
            e.Handled = True
        End If
    End Sub

    Protected Sub Object_Drop(sender As Object, e As DragEventArgs)
        Dim Pos = e.GetPosition(ObjectListBox)
        Dim Def As ZDef = e.Data.GetData(GetType(ZDef))

        Dim Args As New List(Of Object)

        Dim ImageW = Map.Scenario.Images(Def.DefaultImage).Image.Width
        Dim ImageH = Map.Scenario.Images(Def.DefaultImage).Image.Height

        Args.Add(CInt(Math.Round(Pos.X - (Def.DefaultW / 2))))
        Args.Add(CInt(Math.Round(Pos.Y + (ImageH / 2) - (Def.DefaultH) + Def.DefaultZ)))

        Dim Obj = ObjectType.BaseType.GetMethod("FromDef").Invoke(Nothing, {Def, Args})

        ObjectCollection.Add(Obj)

        ObjectListBox.SelectedItem = Obj
        ObjectListBox.Focus()

        'Dim Obj As BaseType = FinishDrop(Def, Args)
        'If Obj IsNot Nothing Then
        '    ObjectCollection.Add(Obj)
        '    ObjectListBox.SelectedItem = Obj
        '    ObjectListBox.Focus()
        'End If

    End Sub

    Public Sub ItemContainer_MouseLeftButtonUp(sender As System.Object, e As System.Windows.Input.MouseButtonEventArgs)
        If Mouse.Captured Is sender Then
            sender.ReleaseMouseCapture()
            If Not _IsDraggingObject Then
                ObjectListBox.SelectedItem = sender.Content
                RaiseCoordinatesUpdated()
            End If


            If _FirstClick Then
                _FirstClick = False
            Else
                If Not _IsDraggingObject And Environment.TickCount - _StartClickTime < 500 Then
                    ItemContainer_MouseDoubleClick(sender, e)
                End If
                _StartClickTime = Nothing
                _FirstClick = True
            End If

            _IsDraggingObject = False
        End If
    End Sub

    Private Sub OnCanExecuteProperties(ByVal sender As Object, ByVal e As CanExecuteRoutedEventArgs)
        If ObjectListBox.SelectedItems.Count > 0 Then
            ' The condition for the command was met.
            e.CanExecute = True
            e.Handled = True
        End If
    End Sub

    Private Sub OnExecuteProperties(ByVal sender As Object, ByVal e As ExecutedRoutedEventArgs)
        Dim Frm = New ObjectProperties()

        Dim ListBoxItem As ListBoxItem = ObjectListBox.ItemContainerGenerator.ContainerFromItem(ObjectListBox.SelectedItem)
        Dim ObjOld As IBaseGeneralObject = ObjectListBox.SelectedItem
        Dim AbsolutePoint = New Point(Canvas.GetLeft(ListBoxItem) + ListBoxItem.ActualWidth + 2, Canvas.GetTop(ListBoxItem) - 30)

        Frm.Left = AbsolutePoint.X
        Frm.Top = AbsolutePoint.Y
        Frm.WindowStartupLocation = WindowStartupLocation.Manual
        Frm.Owner = Window.GetWindow(Me)

        Dim Map As MapData = Me.DataContext

        Dim ObjClone As IBaseGeneralObject = ObjOld.Clone
        Frm.DataContext = ObjClone

        If Frm.ShowDialog() = True Then
            'ObjClone.UpdatePosition(ObjClone.Args(0).Value, ObjClone.Args(1).Value)
            'UndoManager.PushUndoState(Map, UndoManager.TypeFlagFromType(ObjectType))

            ObjectCollection(ObjectListBox.SelectedIndex) = ObjClone
            ObjectListBox.SelectedItem = ObjClone

            If ObjClone.NamedSlot <> ObjOld.NamedSlot Then
                If ObjClone.NamedSlot IsNot Nothing AndAlso Not Map.Scenario.NamedSlots.Contains(ObjClone.NamedSlot) Then
                    Map.Scenario.NamedSlots.Add(ObjClone.NamedSlot)
                End If
                If ObjOld.NamedSlot IsNot Nothing Then
                    Map.Scenario.NamedSlots.Remove(ObjOld.NamedSlot)
                End If
            End If
        End If

        ' Work was done for this command. Mark the event as handled.
        e.Handled = True
    End Sub

    Private Sub OnCanExecuteDelete(ByVal sender As Object, ByVal e As CanExecuteRoutedEventArgs)
        If ObjectListBox.SelectedItems.Count > 0 Then
            ' The condition for the command was met.
            e.CanExecute = True
            e.Handled = True
        End If
    End Sub

    Private Sub OnExecuteDelete(ByVal sender As Object, ByVal e As ExecutedRoutedEventArgs)
        Dim ObjsToRemove = New List(Of IBaseGeneralObject)(ObjectListBox.SelectedItems.Cast(Of IBaseGeneralObject))
        For Each Obj In ObjsToRemove
            ObjectCollection.Remove(Obj)
        Next
    End Sub

    Private Sub ObjectCanvas_PreviewMouseDown(sender As Object, e As MouseButtonEventArgs) Handles ObjectListBox.PreviewMouseDown
        If e.RightButton = MouseButtonState.Pressed Then
            Dim Pos = e.GetPosition(sender)
            Dim ObjectsFound As New Collection(Of IBaseGeneralObject)
            For Each Obj In ObjectCollection
                Dim ListBoxItem As ListBoxItem = ObjectListBox.ItemContainerGenerator.ContainerFromItem(Obj)
                Dim Bounds = New Rect(Canvas.GetLeft(ListBoxItem), Canvas.GetTop(ListBoxItem), ListBoxItem.ActualWidth, ListBoxItem.ActualHeight)
                If Bounds.Contains(Pos) Then
                    ObjectsFound.Add(Obj)
                End If
            Next
            ObjectsUnderClick = ObjectsFound
            If ObjectsFound.Count = 1 Then
                ObjectListBox.SelectedItem = ObjectsFound(0)
                Dim ListBoxItem As ListBoxItem = ObjectListBox.ItemContainerGenerator.ContainerFromItem(ObjectListBox.SelectedItem)
                ListBoxItem.Focus()

                Dim Args = New SelectionChangeRequestedArgs(MapLayer.SelectionChangeRequested, Map)
                MyBase.RaiseEvent(Args)

                e.Handled = True
            End If
        End If
    End Sub

    Private Sub SelectMenuItem_Click(sender As Object, e As RoutedEventArgs)
        ObjectListBox.SelectedItem = sender.Tag
    End Sub

    Public Overrides Sub DeselectAll()
        ObjectListBox.SelectedItem = Nothing
    End Sub

    Private Sub UserControl_MouseDoubleClick(sender As Object, e As MouseButtonEventArgs)
        ApplicationCommands.Properties.Execute(Nothing, Me)
    End Sub
End Class
