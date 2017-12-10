Imports Revsoft.TextEditor

Public Class ScriptEditor
    Inherits TextEditorControl

    Public Sub New()

    End Sub

    Private Sub InitializeComponent()
        Me.SuspendLayout()
        '
        'textAreaPanel
        '
        Me.textAreaPanel.Size = New System.Drawing.Size(440, 290)
        '
        'ScriptEditor
        '
        Me.Name = "ScriptEditor"
        Me.Size = New System.Drawing.Size(440, 290)
        Me.ResumeLayout(False)

    End Sub
End Class
