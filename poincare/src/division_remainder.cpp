#include <poincare/division_remainder.h>
#include <poincare/infinity.h>
#include <poincare/layout_helper.h>
#include <poincare/rational.h>
#include <poincare/serialization_helper.h>
#include <poincare/undefined.h>
#include <cmath>

namespace Poincare {

Layout DivisionRemainderNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Prefix(DivisionRemainder(this), floatDisplayMode, numberOfSignificantDigits, name());
}

int DivisionRemainderNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, name());
}

Expression DivisionRemainderNode::shallowReduce(Context & context, Preferences::AngleUnit angleUnit) {
  return DivisionRemainder(this).shallowReduce(context, angleUnit);
}

template<typename T>
Evaluation<T> DivisionRemainderNode::templatedApproximate(Context& context, Preferences::AngleUnit angleUnit) const {
  Evaluation<T> f1Input = childAtIndex(0)->approximate(T(), context, angleUnit);
  Evaluation<T> f2Input = childAtIndex(1)->approximate(T(), context, angleUnit);
  T f1 = f1Input.toScalar();
  T f2 = f2Input.toScalar();
  if (std::isnan(f1) || std::isnan(f2) || f1 != (int)f1 || f2 != (int)f2) {
    return Complex<T>::Undefined();
  }
  return Complex<T>(std::round(f1-f2*std::floor(f1/f2)));
}
DivisionRemainder::DivisionRemainder() : Expression(TreePool::sharedPool()->createTreeNode<DivisionRemainderNode>()) {}

Expression DivisionRemainder::shallowReduce(Context & context, Preferences::AngleUnit angleUnit) {
  {
    Expression e = Expression::defaultShallowReduce(context, angleUnit);
    if (e.isUndefined()) {
      return e;
    }
  }
  Expression c0 = childAtIndex(0);
  Expression c1 = childAtIndex(1);
#if MATRIX_EXACT_REDUCING
  if (c0.type() == ExpressionNode::Type::Matrix || c1.type() == ExpressionNode::Type::Matrix) {
    return Undefined();
  }
#endif
  if (c0.type() == ExpressionNode::Type::Rational) {
    Rational r0 = static_cast<Rational &>(c0);
    if (!r0.integerDenominator().isOne()) {
      Expression result = Undefined();
      replaceWithInPlace(result);
      return result;
    }
  }
  if (c1.type() == ExpressionNode::Type::Rational) {
    Rational r1 = static_cast<Rational &>(c1);
    if (!r1.integerDenominator().isOne()) {
      Expression result = Undefined();
      replaceWithInPlace(result);
      return result;
    }
  }
  if (c0.type() != ExpressionNode::Type::Rational || c1.type() != ExpressionNode::Type::Rational) {
    return *this;
  }
  Rational r0 = static_cast<Rational &>(c0);
  Rational r1 = static_cast<Rational &>(c1);

  Integer a = r0.signedIntegerNumerator();
  Integer b = r1.signedIntegerNumerator();
  if (b.isZero()) {
    Expression result = Infinity(a.isNegative());
    replaceWithInPlace(result);
    return result;
  }
  Integer result = Integer::Division(a, b).remainder;
  assert(!result.isInfinity());
  Expression rationalResult = Rational(result);
  replaceWithInPlace(rationalResult);
  return rationalResult;
}

}
