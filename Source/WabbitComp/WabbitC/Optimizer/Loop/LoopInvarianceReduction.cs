﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using WabbitC.Model;
using WabbitC.Model.Statements;
using WabbitC.Model.Statements.Math;

namespace WabbitC.Optimizer.Loop
{
	static class LoopInvarianceReduction
	{
		public static void Optimize(ref Module module)
		{
			var functions = module.GetFunctionEnumerator();
			while (functions.MoveNext())
			{
				Block block = functions.Current.Code;
				OptimizeBlock(ref block);
			}
		}

		public static void OptimizeBlock(ref Block block)
		{
			var statements = from Statement st in block where st is ILoop select st;
			foreach (ILoop statement in statements)
			{
                var body = statement.Body;
				OptimizeLoopBlock(ref body);
			}
		}

		public static void OptimizeLoopBlock(ref Block block)
		{
			var blocks = BasicBlock.GetBasicBlocks(block);
			Dictionary<Declaration, bool> variantDecls = new Dictionary<Declaration, bool>();
			foreach (var decl in block.Declarations)
			{
				variantDecls.Add(decl, false);
			}
			var statements = from Statement st in block select st;
			bool hasChanged = false;
			do
			{
				foreach (var statement in statements)
				{
					foreach (var modified in statement.GetModifiedDeclarations())
					{
						var refed = statement.GetReferencedDeclarations();
						bool variant = false;
						foreach (var refedvar in refed)
						{
							if (variantDecls.ContainsKey(refedvar))
								variant |= variantDecls[refedvar];
							else
								variantDecls.Add(refedvar, false);
						}
						if (variant)
						{
							variantDecls[modified] = true;
							hasChanged = true;
						}
					}
				}
			} while (hasChanged);
			
		}
	}
}
