﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using WabbitC.Model.Types;
using System.Diagnostics;

namespace WabbitC.Model.Statements.Math
{
    class Div : MathStatement, IMathOperator
    {
        public Div()
        {
        }

        public Div(Declaration lValue, Datum rValue)
        {
            LValue = lValue;
            Operator = Token.DivOperatorToken;
            RValue = rValue;
        }

        #region IMathOperator Members

        public Token GetHandledOperator()
        {
            return Token.DivOperatorToken;
        }

        Immediate IMathOperator.Apply()
        {
			var imm = RValue as Immediate;
			if (imm == null)
				return null;
			LValue.ConstValue.Value = (LValue.ConstValue.Value / imm.Value).Eval()[0].Token;
			return LValue.ConstValue;
        }

        #endregion
    }
}
