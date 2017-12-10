﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WabbitC.Model.Statements
{
	class Dec : Statement
	{
        public Declaration Decl { get; set; }
		public Dec(Declaration decl)
		{
			Decl = decl;
		}

		public override ISet<Declaration> GetModifiedDeclarations()
		{
			return new HashSet<Declaration>() { Decl };
		}

		public override ISet<Declaration> GetReferencedDeclarations()
		{
			return new HashSet<Declaration>() { Decl };
		}

		public override string ToString()
		{
			return Decl + "--;";
		}

		public override string ToAssemblyString()
		{
			return "dec " + Decl;
		}
	}
}
