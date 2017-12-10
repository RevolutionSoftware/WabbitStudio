﻿// Copyright (c) AlphaSierraPapa for the SharpDevelop Team (for details please see \doc\copyright.txt)
// This code is distributed under the GNU LGPL (for details please see \doc\license.txt)

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using Revsoft.Wabbitcode.AvalonEditExtension.Snippets;
using Revsoft.Wabbitcode.AvalonEditExtension.Interface;

namespace Revsoft.Wabbitcode.AvalonEditExtension
{
	/// <summary>
	/// Wraps the CodeEditor class to provide the ITextEditor interface.
	/// </summary>
	sealed class CodeEditorAdapter : CodeCompletionEditorAdapter
	{
		readonly CodeEditor codeEditor;
		
		public CodeEditorAdapter(CodeEditor codeEditor, CodeEditorView textEditor) : base(textEditor)
		{
			if (codeEditor == null)
				throw new ArgumentNullException("codeEditor");
			this.codeEditor = codeEditor;
		}
		
		public override string FileName {
			get { return codeEditor.FileName; }
		}
		
		public override ITextEditor PrimaryView {
			get { return codeEditor.PrimaryTextEditorAdapter; }
		}
		
		internal void FileNameChanged()
		{
			// update properties set by languageBinding
			this.TextEditor.TextArea.IndentationStrategy = new OptionControlledIndentationStrategy(this, languageBinding.FormattingStrategy);
		}
		
		sealed class OptionControlledIndentationStrategy : IndentationStrategyAdapter
		{
			public OptionControlledIndentationStrategy(ITextEditor editor, IFormattingStrategy formattingStrategy)
				: base(editor, formattingStrategy)
			{
			}

            public override void IndentLine(ICSharpCode.AvalonEdit.Document.TextDocument document, ICSharpCode.AvalonEdit.Document.DocumentLine line)
			{
				if (CodeEditorOptions.Instance.UseSmartIndentation)
					base.IndentLine(document, line);
				else
					new DefaultIndentationStrategy().IndentLine(document, line);
			}
			
			// Do not override IndentLines: it is called only when smart indentation is explicitly requested by the user (Ctrl+I),
			// so we keep it enabled even when the option is set to false.
		}
		
		public override IEnumerable<ICompletionItem> GetSnippets()
		{
			CodeSnippetGroup g = SnippetManager.Instance.FindGroup(Path.GetExtension(this.FileName));
			if (g != null) {
				return g.Snippets.Select(s => s.CreateCompletionItem(this));
			} else {
				return base.GetSnippets();
			}
		}
	}
}
