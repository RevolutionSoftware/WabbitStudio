Class ScriptLayer
    Inherits MapLayer

    Public Overrides Property LayerType As LayerType
        Get
            Return WPFZ80MapEditor.LayerType.ScriptLayer
        End Get
        Set(value As LayerType)

        End Set
    End Property
End Class
