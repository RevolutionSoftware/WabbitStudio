﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Diagnostics;

namespace WabbitC.Model.Types
{
    class FunctionType : Type
    {
        public enum CallConvention
        {
            CalleeSave,     //__stdcall
            CallerSave,     //__cdecl
        }
        public Type ReturnType;
        public List<Declaration> Params;
        public CallConvention CallingConvention = CallConvention.CalleeSave;
		public bool IsRecursive = false;
		bool useStack = false;
		public bool UseStack
		{
			get
			{
				if (IsRecursive)
					return true;
				return useStack;
			}
			set
			{
				useStack = value;
			}
		}

		public FunctionType()
		{
		}

		public bool Is16()
		{
			bool Is16 = false;
			if (this.ReturnType.IndirectionLevels > 0)
			{
				Is16 = true;
			}
			else if (this.ReturnType.Size > 1)
			{
				Is16 = true;
			}
			return Is16;
		}

        public FunctionType(ref List<Token>.Enumerator tokens, Type returnType)
        {
            this.Size = 2;

            Debug.Assert(tokens.Current.Type == TokenType.OpenParen);
            
            tokens.MoveNext();

            this.ReturnType = returnType;

            Params = new List<Declaration>();
            while (tokens.Current.Type != TokenType.CloseParen)
            {
                Type type = TypeHelper.ParseType(ref tokens);
                string name = tokens.Current.Text;
                tokens.MoveNext();

                Params.Add(new Declaration(type, name));

                Debug.Assert(tokens.Current.Text == "," || tokens.Current.Text == ")");

                if (tokens.Current.Text == ",")
                    tokens.MoveNext();
            }

            tokens.MoveNext();
        }

        public override string ToString()
        {
            string result = ReturnType + " (*)(";
            for (int i = 0; i < Params.Count; i++)
            {
                result += Params[i].Type + " " + Params[i].Name;
                if (i != Params.Count - 1)
                    result += ", ";
            }
            result += ")";
            return result;
        }

        public override string ToDeclarationString(string DeclName)
        {
            return this.ToString().Replace("(*)", DeclName);
        }
    }
}
