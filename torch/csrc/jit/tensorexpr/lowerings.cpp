#include <torch/csrc/jit/frontend/function_schema_parser.h>
#include <torch/csrc/jit/tensorexpr/ir_simplifier.h>
#include <torch/csrc/jit/tensorexpr/lowerings.h>
#include <torch/csrc/jit/tensorexpr/operators/operators.h>

namespace torch {
namespace jit {
namespace tensorexpr {

FunctionSchemaMap<NNCLoweringFunction>& getNNCLoweringRegistry() {
  static FunctionSchemaMap<NNCLoweringFunction> lowering_registry_;
  return lowering_registry_;
}

NNCLoweringFunction getStandardLoweringFor(const std::string& schema_str) {
  const auto& lowerings = getNNCLoweringRegistry();
  if (auto l = lowerings.find(parseSchema(schema_str))) {
    return *l;
  }
  return nullptr;
}

RegisterNNCLoweringsFunction::RegisterNNCLoweringsFunction(
    const std::vector<std::string>& schemas,
    NNCLoweringFunction fn) {
  for (const auto& schema_str : schemas) {
    getNNCLoweringRegistry().insert(parseSchema(schema_str), fn);
  }
}

namespace {
RegisterNNCLoweringsFunction aten_dropout(
    {"aten::dropout(Tensor input, float p, bool train) -> (Tensor)"},
    computeNoop);

// TODO: convert to schema, add a test
// RegisterNNCLoweringsFunction prepacked_conv2d_clamp_run(
//     {"prepacked::conv2d_clamp_run"},
//     computePrepackedConv2dClampRun);

// TODO: convert to schema, add a test
// RegisterNNCLoweringsFunction prepacked_linear_clamp_run(
//     {"prepacked::linear_clamp_run"},
//     computePrepackedLinearClampRun);

RegisterNNCLoweringsFunction aten_sub(
    {"aten::sub.Scalar(Tensor self, Scalar other, Scalar alpha=1) -> (Tensor)",
     "aten::sub.Tensor(Tensor self, Tensor other, *, Scalar alpha=1) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      auto sub_lambda = [](const ExprHandle& lhs, const ExprHandle& rhs) {
        // NB: sub isn't supported on boolean, no need to promote to integer.
        return lhs - rhs;
      };
      TORCH_INTERNAL_ASSERT(
          inputs.size() == 2 || inputs.size() == 3,
          buildErrorMessage("Invalid number of input operands"));
      return (inputs.size() > 2)
          ? computeTwoOperandWithAlpha(
                "aten_sub", inputs, outputShape, outputType, sub_lambda)
          : computeTwoOperand(
                "aten_sub", inputs, outputShape, outputType, sub_lambda);
    });

RegisterNNCLoweringsFunction aten_mul(
    {"aten::mul.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::mul.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_mul",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return boolToInteger(lhs) * boolToInteger(rhs);
          });
    });

RegisterNNCLoweringsFunction aten_div(
    {"aten::div.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::div.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_div",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return promoteIntegerToDefaultType(lhs) /
                promoteIntegerToDefaultType(rhs);
          });
    });

RegisterNNCLoweringsFunction aten___and__(
    {"aten::__and__.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::__and__.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_and",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return boolToInteger(lhs) & boolToInteger(rhs);
          });
    });

RegisterNNCLoweringsFunction aten___or__(
    {"aten::__or__.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::__or__.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_or",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return boolToInteger(lhs) | boolToInteger(rhs);
          });
    });

RegisterNNCLoweringsFunction aten___xor__(
    {"aten::__xor__.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::__xor__.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_xor",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return boolToInteger(lhs) ^ boolToInteger(rhs);
          });
    });

RegisterNNCLoweringsFunction aten___lshift__(
    {"aten::__lshift__.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_lshift",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return lhs << rhs;
          });
    });

RegisterNNCLoweringsFunction aten___rshift__(
    {"aten::__rshift__.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_rshift",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return lhs >> rhs;
          });
    });

RegisterNNCLoweringsFunction aten_eq(
    {"aten::eq.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::eq.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_eq",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return cast<bool>(lhs == rhs);
          });
    });

RegisterNNCLoweringsFunction aten_ne(
    {"aten::ne.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::ne.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_ne",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return cast<bool>(lhs != rhs);
          });
    });

RegisterNNCLoweringsFunction aten_ge(
    {"aten::ge.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::ge.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_ge",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return cast<bool>(lhs >= rhs);
          });
    });

RegisterNNCLoweringsFunction aten_gt(
    {"aten::gt.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::gt.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_gt",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return cast<bool>(lhs > rhs);
          });
    });

RegisterNNCLoweringsFunction aten_le(
    {"aten::le.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::le.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_le",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return cast<bool>(lhs <= rhs);
          });
    });

RegisterNNCLoweringsFunction aten_lt(
    {"aten::lt.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::lt.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_lt",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return cast<bool>(lhs < rhs);
          });
    });

RegisterNNCLoweringsFunction aten_min_pointwise(
    {"aten::min.other(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_min",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return Min::make(boolToInteger(lhs), boolToInteger(rhs), false);
          });
    });

RegisterNNCLoweringsFunction aten_max_pointwise(
    {"aten::max.other(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_max",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return Max::make(boolToInteger(lhs), boolToInteger(rhs), false);
          });
    });

RegisterNNCLoweringsFunction aten_masked_fill(
    {"aten::masked_fill.Scalar(Tensor self, Tensor mask, Scalar value) -> (Tensor)",
     "aten::masked_fill.Tensor(Tensor self, Tensor mask, Tensor value) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeThreeOperand(
          "aten_masked_fill",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& input,
             const ExprHandle& mask,
             const ExprHandle& value) {
            // value needs to promote to input, not vice versa
            auto val = promoteToDtype(value, input.dtype().scalar_type());
            return ifThenElse(mask, val, input);
          },
          /*promote_inputs*/ false);
    });
RegisterNNCLoweringsFunction aten_clamp(
    {"aten::clamp(Tensor self, Scalar? min=None, Scalar? max=None) -> (Tensor)",
     "aten::clamp.Tensor(Tensor self, Tensor? min=None, Tensor? max=None) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      bool noMin = false;
      bool noMax = false;
      if (c10::get_if<ArgNone>(&inputs[1])) {
        noMin = true;
      }

      if (c10::get_if<ArgNone>(&inputs[2])) {
        noMax = true;
      }

      return computeThreeOperand(
          "aten_clamp",
          inputs,
          outputShape,
          outputType,
          [noMin, noMax](
              const ExprHandle& in,
              const ExprHandle& min,
              const ExprHandle& max) {
            auto cast = [&](const ExprHandle& e) {
              return Cast::make(in.dtype(), e);
            };

            if (noMin && noMax) {
              return in;
            } else if (noMin) {
              auto cmax = cast(max);
              return CompareSelect::make(in, cmax, cmax, in, kGT);
            } else if (noMax) {
              auto cmin = cast(min);
              return CompareSelect::make(in, cmin, cmin, in, kLT);
            } else {
              auto cmax = cast(max);
              auto cmin = cast(min);
              return clamp(cmin, cmax, in);
            }
          },
          false /* promote_inputs */);
    });

RegisterNNCLoweringsFunction aten_addcmul(
    {"aten::addcmul(Tensor self, Tensor tensor1, Tensor tensor2, *, Scalar value=1) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeFourOperand(
          "aten_addcmul",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a0,
             const ExprHandle& a1,
             const ExprHandle& a2,
             const ExprHandle& a3) { return a0 + a3 * a1 * a2; });
    });

RegisterNNCLoweringsFunction aten_sigmoid(
    {"aten::sigmoid(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_sigmoid",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return sigmoid(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_reciprocal(
    {"aten::reciprocal(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_reciprocal",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) { return ExprHandle(1.0f) / a; });
    });

RegisterNNCLoweringsFunction aten_neg(
    {"aten::neg(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_neg", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return ExprHandle(-0) - a;
          });
    });

RegisterNNCLoweringsFunction aten_isnan(
    {"aten::isnan(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_isnan",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            if (!a.dtype().is_floating_point()) {
              return IntImm::make(0);
            }
            return isnan(a);
          });
    });

RegisterNNCLoweringsFunction aten_relu(
    {"aten::relu(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_relu",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            auto zero = Cast::make(a.dtype(), 0);
            return CompareSelect::make(a, zero, zero, a, kLT);
          });
    });

RegisterNNCLoweringsFunction aten_leaky_relu(
    {"aten::leaky_relu(Tensor self, Scalar negative_slope=0.01) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_leaky_relu",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a, const ExprHandle& negative_slope) {
            auto neg_slope = Cast::make(a.dtype(), negative_slope);
            auto zero = Cast::make(a.dtype(), 0);
            auto one = Cast::make(a.dtype(), 1);
            auto cs = CompareSelect::make(a, zero, one, neg_slope, kGT);
            return a * cs;
          });
    });

RegisterNNCLoweringsFunction aten_relu6(
    {"aten::relu6(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_relu6",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            auto zero = Cast::make(a.dtype(), 0);
            auto six = Cast::make(a.dtype(), 6.);
            return clamp(zero, six, a);
          });
    });

RegisterNNCLoweringsFunction aten_gelu(
    {"aten::gelu(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_gelu",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            auto m_sqrt1_2 = Cast::make(a.dtype(), M_SQRT1_2);
            auto one = Cast::make(a.dtype(), 1.);
            auto point_five = Cast::make(a.dtype(), .5);
            return a * point_five * (one + erf(a * m_sqrt1_2));
          });
    });

RegisterNNCLoweringsFunction aten_batch_norm(
    {"aten::batch_norm(Tensor input, Tensor? weight, Tensor? bias, Tensor? running_mean, Tensor? running_var, bool training, float momentum, float eps, bool cudnn_enabled) -> (Tensor)"},
    computeBatchNorm);

RegisterNNCLoweringsFunction aten_log(
    {"aten::log(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_log", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return log(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_log10(
    {"aten::log10(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_log10",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return log10(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_log1p(
    {"aten::log1p(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_log1p",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return log1p(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_log2(
    {"aten::log2(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_log2",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return log2(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_exp(
    {"aten::exp(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_exp", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return exp(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_expm1(
    {"aten::expm1(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_expm1",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return expm1(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_erf(
    {"aten::erf(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_erf", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return erf(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_erfc(
    {"aten::erfc(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_erfc",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return erfc(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_cos(
    {"aten::cos(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_cos", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return cos(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_sin(
    {"aten::sin(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_sin", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return sin(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_tan(
    {"aten::tan(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_tan", inputs, outputShape, outputType, [](const ExprHandle& a) {
            return tan(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_type_as(
    {"aten::type_as(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      const BufHandle& rhs = c10::get<BufHandle>(inputs[1]);
      auto dtype = rhs.dtype();
      return computeOneOperand(
          "aten_type_as",
          inputs,
          outputShape,
          outputType,
          [dtype](const ExprHandle& lhs) { return Cast::make(dtype, lhs); });
    });

RegisterNNCLoweringsFunction aten_pow(
    {"aten::pow.Tensor_Scalar(Tensor self, Scalar exponent) -> (Tensor)",
     "aten::pow.Tensor_Tensor(Tensor self, Tensor exponent) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_pow",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            if (!rhs.node()->isConstant()) {
              return pow(lhs, rhs);
            }
            double val =
                immediateAs<double>(IRSimplifier::simplify(rhs.node()));

            if (val == 1.0f) {
              return lhs;
            } else if (val == 2.0f) { // NOLINT
              return lhs * lhs;
            } else if (val == 3.0f) { // NOLINT
              return (lhs * lhs) * lhs;
            } else if (val == 4.0f) { // NOLINT
              ExprHandle tmp = lhs * lhs;
              return tmp * tmp;
            } else if (val == 0.5f) { // NOLINT
              return sqrt(lhs);
            } else if (val == 0.0f) {
              return ExprHandle(1.0f);
            } else if (val == -0.5f) { // NOLINT
              return rsqrt(lhs);
            } else if (val == -1.0f) {
              return ExprHandle(1.0f) / lhs;
            } else if (val == -2.0f) { // NOLINT
              return ExprHandle(1.0f) / (lhs * lhs);
            }
            return pow(lhs, rhs);
          });
    });

RegisterNNCLoweringsFunction aten_fmod(
    {"aten::fmod.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::fmod.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_fmod",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return fmod(promoteHalfToFloat(lhs), promoteHalfToFloat(rhs));
          });
    });

RegisterNNCLoweringsFunction aten_lerp(
    {"aten::lerp.Scalar(Tensor self, Tensor end, Scalar weight) -> (Tensor)",
     "aten::lerp.Tensor(Tensor self, Tensor end, Tensor weight) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeThreeOperand(
          "aten_lerp",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a,
             const ExprHandle& end,
             const ExprHandle& weight) { return a + weight * (end - a); });
    });

RegisterNNCLoweringsFunction aten_remainder(
    {"aten::remainder.Scalar(Tensor self, Scalar other) -> (Tensor)",
     "aten::remainder.Scalar_Tensor(Scalar self, Tensor other) -> (Tensor)",
     "aten::remainder.Tensor(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      auto imodImpl = [](const ExprHandle& lhs, const ExprHandle& rhs) {
        return Mod::make(lhs, rhs);
      };
      auto fmodImpl = [](const ExprHandle& lhs, const ExprHandle& rhs) {
        auto lhs_t = promoteHalfToFloat(lhs);
        auto rhs_t = promoteHalfToFloat(rhs);
        return fmod((rhs_t + fmod(lhs_t, rhs_t)), rhs_t);
      };
      {
        auto const& shape =
            broadcastShapes(valueShape(inputs[0]), valueShape(inputs[1]));
        return Compute(
            "aten_remainder",
            c10::fmap<DimArg>(shape),
            [&](const std::vector<VarHandle>& axes) {
              std::vector<ExprHandle> indices(axes.begin(), axes.end());
              std::vector<ExprHandle> exprInputs = {
                  tensorOrConstant(inputs[0], indices),
                  tensorOrConstant(inputs[1], indices),
              };

              promoteInputs(exprInputs);
              bool allInt = true;
              for (auto& e : exprInputs) {
                if (e.dtype().is_floating_point()) {
                  allInt = false;
                  break;
                }
              }
              if (allInt) {
                return demoteOutput(
                    imodImpl(exprInputs[0], exprInputs[1]), outputType);
              } else {
                return demoteOutput(
                    fmodImpl(exprInputs[0], exprInputs[1]), outputType);
              }
            });
      }
    });

RegisterNNCLoweringsFunction prim_ConstantChunk(
    {"prim::ConstantChunk(...) -> (...)"},
    computeChunk);

RegisterNNCLoweringsFunction aten_acos(
    {"aten::acos(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_acos",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return acos(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_asin(
    {"aten::asin(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_asin",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return asin(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_cosh(
    {"aten::cosh(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_cosh",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return cosh(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_sinh(
    {"aten::sinh(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_sinh",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return sinh(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_atan(
    {"aten::atan(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_atan",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return atan(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_atan2(
    {"aten::atan2(Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_atan2",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& lhs, const ExprHandle& rhs) {
            return atan2(
                promoteIntegerToDefaultType(lhs),
                promoteIntegerToDefaultType(rhs));
          });
    });

RegisterNNCLoweringsFunction aten_tanh(
    {"aten::tanh(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_tanh",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return tanh(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_hardtanh(
    {"aten::hardtanh(Tensor self, Scalar min_val=-1, Scalar max_val=1) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeThreeOperand(
          "aten_hardtanh",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a,
             const ExprHandle& min_val,
             const ExprHandle& max_val) {
            auto mm = CompareSelect::make(a, min_val, min_val, a, kLT);
            return CompareSelect::make(mm, max_val, max_val, mm, kGT);
          });
    });

RegisterNNCLoweringsFunction aten_softplus(
    {"aten::softplus(Tensor self, Scalar beta=1, Scalar threshold=20) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeThreeOperand(
          "aten_softplus",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a,
             const ExprHandle& beta,
             const ExprHandle& threshold) {
            auto beta_promoted = Cast::make(a.dtype(), beta);
            auto threshold_promoted = Cast::make(a.dtype(), threshold);
            auto beta_a = beta_promoted * a;
            return CompareSelect::make(
                beta_a,
                threshold_promoted,
                a,
                log1p(exp(beta_a)) / beta_promoted,
                kGT);
          });
    });

RegisterNNCLoweringsFunction aten_hardsigmoid(
    {"aten::hardsigmoid(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_hardsigmoid",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            auto zero = Cast::make(a.dtype(), 0.0);
            auto three = Cast::make(a.dtype(), 3.0);
            auto six = Cast::make(a.dtype(), 6.0);
            return clamp(zero, six, a + three) / six;
          });
    });

RegisterNNCLoweringsFunction aten_hardswish(
    {"aten::hardswish(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_hardswish",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            //  x * torch.clamp(x + 3.0, 0.0, 6.0) / 6.0
            auto zero = Cast::make(a.dtype(), 0.);
            auto three = Cast::make(a.dtype(), 3.);
            auto six = Cast::make(a.dtype(), 6.);

            return a * clamp(zero, six, a + three) / six;
          });
    });

RegisterNNCLoweringsFunction aten_hardshrink(
    {"aten::hardshrink(Tensor self, Scalar lambd=0.5) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTwoOperand(
          "aten_hardshrink",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a, const ExprHandle& lambd) {
            auto pos_clambd = Cast::make(a.dtype(), lambd);
            auto neg_clambd =
                Cast::make(a.dtype(), ExprHandle(-0)) - pos_clambd;
            auto zero = Cast::make(a.dtype(), 0);
            auto mm = CompareSelect::make(a, neg_clambd, a, zero, kLT);
            return CompareSelect::make(a, pos_clambd, a, mm, kGT);
          });
    });

RegisterNNCLoweringsFunction aten_sqrt(
    {"aten::sqrt(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_sqrt",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return tensorexpr::sqrt(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_rsqrt(
    {"aten::rsqrt(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_rsqrt",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return rsqrt(promoteIntegerToDefaultType(a));
          });
    });

RegisterNNCLoweringsFunction aten_abs(
    {"aten::abs(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_abs",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return tensorexpr::abs(promoteHalfToFloat(a));
          },
          kIntegralTypes | kFloatingPointTypes | kBoolType);
    });

RegisterNNCLoweringsFunction aten_sign(
    {"aten::sign(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) { return computeSign(inputs, outputShape); });

RegisterNNCLoweringsFunction aten_ceil(
    {"aten::ceil(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_ceil",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) { return ceil(a); });
    });

RegisterNNCLoweringsFunction aten_floor(
    {"aten::floor(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_floor",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) { return floor(a); });
    });

RegisterNNCLoweringsFunction aten_round(
    {"aten::round(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_round",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) { return round(a); });
    });

RegisterNNCLoweringsFunction aten_trunc(
    {"aten::trunc(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_trunc",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) { return trunc(a); });
    });

RegisterNNCLoweringsFunction aten__cast_Float(
    {"aten::_cast_Float(Tensor self, bool non_blocking=False) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_cast_float",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) { return cast<float>(a); });
    });

RegisterNNCLoweringsFunction aten_to(
    {"aten::to.dtype(Tensor(a) self, int dtype, bool non_blocking=False, bool copy=False, int? memory_format=None) -> (Tensor(a))",
     "aten::to.dtype_layout(Tensor(a) self, *, int? dtype=None, int? layout=None, Device? device=None, bool? pin_memory=None, bool non_blocking=False, bool copy=False, int? memory_format=None) -> (Tensor(a))",
     "aten::to.device(Tensor(a) self, Device device, int dtype, bool non_blocking=False, bool copy=False, int? memory_format=None) -> (Tensor(a))",
     "aten::to.prim_Device(Tensor(a) self, Device? device, int? dtype=None, bool non_blocking=False, bool copy=False) -> Tensor(a|b)",
     "aten::to.prim_dtype(Tensor(a) self, int? dtype=None, bool non_blocking=False, bool copy=False) -> Tensor(a|b)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      // see handling of aten::to in tensorexpr_fuser.cpp for why we only
      // need to handle the first input
      return computeOneOperand(
          "aten_to",
          {inputs[0]},
          outputShape,
          outputType,
          [outputType](const ExprHandle& a) {
            TORCH_INTERNAL_ASSERT(
                outputType, buildErrorMessage("Output type is null."));
            return Cast::make(ToDtype(*outputType), a);
          });
    });

RegisterNNCLoweringsFunction aten_threshold(
    {"aten::threshold(Tensor self, Scalar threshold, Scalar value) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeThreeOperand(
          "aten_threshold",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a,
             const ExprHandle& threshold,
             const ExprHandle& value) {
            return ifThenElse(CompareSelect::make(a, threshold, kLE), value, a);
          });
    });

RegisterNNCLoweringsFunction aten_where(
    {"aten::where.ScalarOther(Tensor condition, Tensor self, Scalar other) -> (Tensor)",
     "aten::where.ScalarSelf(Tensor condition, Scalar self, Tensor other) -> (Tensor)",
     "aten::where.self(Tensor condition, Tensor self, Tensor other) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeConditionWithTwoOperand(
          "aten_where",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a0, const ExprHandle& a1, const ExprHandle& a2) {
            return ifThenElse(a0, a1, a2);
          });
    });

RegisterNNCLoweringsFunction aten_frac(
    {"aten::frac(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_frac",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            auto aa = promoteHalfToFloat(a);
            return aa - floor(aa);
          },
          kFloatingPointTypes);
    });

RegisterNNCLoweringsFunction aten_lgamma(
    {"aten::lgamma(Tensor self) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeOneOperand(
          "aten_lgamma",
          inputs,
          outputShape,
          outputType,
          [](const ExprHandle& a) {
            return lgamma(promoteIntegerToDefaultType(a));
          });
    });

// TODO: convert to schema, add a test
// RegisterNNCLoweringsFunction aten_rand_like(
//     {"aten::rand_like"},
//     [](const std::vector<ArgValue>& inputs,
//        const std::vector<ExprHandle>& outputShape,
//        const c10::optional<ScalarType>& outputType,
//        at::Device device) {
//       return computeOneOperand(
//           "aten_rand_like",
//           inputs,
//           outputShape,
//           outputType,
//           [](const ExprHandle& a) {
//             return Intrinsics::make(IntrinsicsOp::kRand, a.dtype());
//           });
//     });

// TODO: convert to schema, add a test
// RegisterNNCLoweringsFunction aten_slice(
//     {"aten::slice"},
//     [](const std::vector<ArgValue>& inputs,
//        const std::vector<ExprHandle>& outputShape,
//        const c10::optional<ScalarType>& outputType,
//        at::Device device) {
//       return Compute(
//           "aten_slice",
//           c10::fmap<DimArg>(outputShape),
//           [&](const std::vector<VarHandle>& axes) {
//             int64_t dim =
//                 at::maybe_wrap_dim(c10::get<int64_t>(inputs[1]),
//                 axes.size());
//             ExprHandle start = constant(inputs[2]);
//             ExprHandle stride = constant(inputs[4]);

//             std::vector<ExprHandle> newAxes(axes.begin(), axes.end());
//             newAxes[dim] = stride * newAxes[dim] + start;
//             return tensorOrConstant(inputs[0], newAxes);
//           });
//     });
RegisterNNCLoweringsFunction aten_unsqueeze(
    {"aten::unsqueeze(Tensor(a) self, int dim) -> (Tensor(a))"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return Compute(
          "aten_unsqueeze",
          c10::fmap<DimArg>(outputShape),
          [&](const std::vector<VarHandle>& axes) {
            int64_t dim = c10::get<int64_t>(inputs[1]);
            if (dim < 0) {
              if (axes.size() == 0) {
                throw malformed_input("axes are zero handling unsqueeze");
              }
              dim += axes.size();
            }
            // To construct an expression for an 'unsqueezed' tensor we need to
            // drop the DIM-th axis, i.e.
            //    unsqueezed_v[i,j,k,l] = v[i,j,l] # dim = 2 - drop index 'k'
            //                 0 1 2 3
            std::vector<ExprHandle> indices;
            int64_t i = 0;
            for (const auto& a : axes) {
              if (i++ != dim) {
                indices.emplace_back(ExprHandle(a.node()));
              }
            }

            return broadcast(c10::get<BufHandle>(inputs[0]), indices);
          });
    });
RegisterNNCLoweringsFunction aten_t(
    {"aten::t(Tensor(a) self) -> (Tensor(a))"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeTranspose(
          {inputs[0], (int64_t)1, (int64_t)0}, outputShape, outputType, device);
    });
RegisterNNCLoweringsFunction aten_transpose(
    {"aten::transpose.int(Tensor(a) self, int dim0, int dim1) -> (Tensor(a))"},
    computeTranspose);
RegisterNNCLoweringsFunction aten_permute(
    {"aten::permute(Tensor(a) self, int[] dims) -> (Tensor(a))"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      auto A = c10::get<BufHandle>(inputs[0]);
      // Trivial case of 0-dim tensors: just a copy of the input
      if (A.ndim() == 0) {
        return Compute(
            "aten_permute",
            c10::fmap<DimArg>(outputShape),
            [&](const std::vector<VarHandle>& axes) {
              std::vector<ExprHandle> empty_indices;
              return A.load(empty_indices);
            });
      }
      auto permute_dims = c10::get<IntList>(inputs[1]);
      return Compute(
          "aten_permute",
          c10::fmap<DimArg>(outputShape),
          [&](const std::vector<VarHandle>& axes) {
            std::vector<VarHandle> new_axes;
            new_axes.resize(axes.size());
            assert(permute_dims.size() == axes.size());
            for (unsigned i = 0; i < axes.size(); i++) {
              auto new_dim = at::maybe_wrap_dim(permute_dims[i], A.ndim());
              new_axes[new_dim] = axes[i];
            }
            return A.load(new_axes);
          });
    });
RegisterNNCLoweringsFunction aten_expand(
    {"aten::expand(Tensor(a) self, int[] size, *, bool implicit=False) -> (Tensor(a))",
     "aten::expand_as(Tensor(a) self, Tensor other) -> (Tensor(a))"},
    computeExpand);

// TODO: convert to schema, add a test
// RegisterNNCLoweringsFunction aten_flatten({"aten::flatten"}, computeFlatten);
RegisterNNCLoweringsFunction aten_view(
    {"aten::reshape(Tensor(a) self, int[] shape) -> (Tensor(a))",
     "aten::reshape_as(Tensor(a) self, Tensor other) -> (Tensor(a))",
     "aten::view(Tensor(a) self, int[] size) -> (Tensor(a))",
     "aten::view_as(Tensor(a) self, Tensor other) -> (Tensor(a))"},
    computeReshape);

// aten::mm is a subset of aten::matmul where both inputs are rank 2
RegisterNNCLoweringsFunction aten_matmul(
    {"aten::mm(Tensor self, Tensor mat2) -> (Tensor)",
     "aten::matmul(Tensor self, Tensor other) -> (Tensor)"},
    computeMatmul);

RegisterNNCLoweringsFunction aten_cat(
    {"aten::cat(Tensor[] tensors, int dim=0) -> (Tensor)"},
    computeCat);

RegisterNNCLoweringsFunction aten_sum(
    {"aten::sum(Tensor self, *, int? dtype=None) -> (Tensor)",
     "aten::sum.dim_IntList(Tensor self, int[1] dim, bool keepdim=False, *, int? dtype=None) -> (Tensor)"},
    computeSum);

RegisterNNCLoweringsFunction aten_softmax(
    {"aten::softmax.int(Tensor self, int dim, int? dtype=None) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeSoftmax(inputs, outputShape, false);
    });

RegisterNNCLoweringsFunction aten_log_softmax(
    {"aten::log_softmax.int(Tensor self, int dim, int? dtype=None) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      return computeSoftmax(inputs, outputShape, true);
    });

RegisterNNCLoweringsFunction aten_conv2d(
    {"aten::conv2d(Tensor input, Tensor weight, Tensor? bias=None, int[2] stride=[1, 1], int[2] padding=[0, 0], int[2] dilation=[1, 1], int groups=1) -> (Tensor)"},
    computeConv2d);

RegisterNNCLoweringsFunction aten_addmm(
    {"aten::addmm(Tensor self, Tensor mat1, Tensor mat2, *, Scalar beta=1, Scalar alpha=1) -> (Tensor)"},
    computeAddMM);

RegisterNNCLoweringsFunction aten_mean(
    {"aten::mean(Tensor self, *, int? dtype=None) -> (Tensor)",
     "aten::mean.dim(Tensor self, int[1] dim, bool keepdim=False, *, int? dtype=None) -> (Tensor)"},
    computeMean);

RegisterNNCLoweringsFunction aten_adaptive_avg_pool2d(
    {"aten::adaptive_avg_pool2d(Tensor self, int[2] output_size) -> (Tensor)"},
    computeAdaptiveAvgPool2d);

RegisterNNCLoweringsFunction aten_add(
    {"aten::add.Scalar(Tensor self, Scalar other, Scalar alpha=1) -> (Tensor)",
     "aten::add.Tensor(Tensor self, Tensor other, *, Scalar alpha=1) -> (Tensor)"},
    [](const std::vector<ArgValue>& inputs,
       const std::vector<ExprHandle>& outputShape,
       const c10::optional<ScalarType>& outputType,
       at::Device device) {
      auto add_lambda = [](const ExprHandle& lhs, const ExprHandle& rhs) {
        return boolToInteger(lhs) + boolToInteger(rhs);
      };
      TORCH_INTERNAL_ASSERT(
          inputs.size() == 2 || inputs.size() == 3,
          buildErrorMessage("Invalid number of input operands"));
      return (inputs.size() > 2)
          ? computeTwoOperandWithAlpha(
                "aten_add", inputs, outputShape, outputType, add_lambda)
          : computeTwoOperand(
                "aten_add", inputs, outputShape, outputType, add_lambda);
    });

} // namespace

} // namespace tensorexpr
} // namespace jit
} // namespace torch
