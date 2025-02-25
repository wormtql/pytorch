Quantization API Reference
-------------------------------

torch.quantization
~~~~~~~~~~~~~~~~~~

This module contains Eager mode quantization APIs.

.. currentmodule:: torch.quantization

Top level APIs
^^^^^^^^^^^^^^

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    quantize
    quantize_dynamic
    quantize_qat
    prepare
    prepare_qat
    convert

Preparing model for quantization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    fuse_modules
    QuantStub
    DeQuantStub
    QuantWrapper
    add_quant_dequant

Utility functions
^^^^^^^^^^^^^^^^^

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    add_observer_
    swap_module
    propagate_qconfig_
    default_eval_fn
    get_observer_dict

torch.quantization.quantize_fx
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module contains FX graph mode quantization APIs (prototype).

.. currentmodule:: torch.quantization.quantize_fx

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    prepare_fx
    prepare_qat_fx
    convert_fx
    fuse_fx

torch (quantization related functions)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This describes the quantization related functions of the `torch` namespace.

.. currentmodule:: torch

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    quantize_per_tensor
    quantize_per_channel
    dequantize

torch.Tensor (quantization related methods)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
>>>>>>> 76f04ad385 (Quantization docs: rewrite API reference to be more automated)
>>>>>>> 37fe4f87ce (Quantization docs: rewrite API reference to be more automated)
>>>>>>> af26f377de (Quantization docs: rewrite API reference to be more automated)

Quantized Tensors support a limited subset of data manipulation methods of the
regular full-precision tensor.

.. currentmodule:: torch.Tensor

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    view
    as_strided
    expand
    flatten
    select
    ne
    eq
    ge
    le
    gt
    lt
    copy_
    clone
    dequantize
    equal
    int_repr
    max
    mean
    min
    q_scale
    q_zero_point
    q_per_channel_scales
    q_per_channel_zero_points
    q_per_channel_axis
    resize_
    sort
    topk


torch.quantization.observer
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module contains observers which are used to collect statistics about
the values observed during calibration (PTQ) or training (QAT).

.. currentmodule:: torch.quantization.observer

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    ObserverBase
    MinMaxObserver
    MovingAverageMinMaxObserver
    PerChannelMinMaxObserver
    MovingAveragePerChannelMinMaxObserver
    HistogramObserver
    PlaceholderObserver
    RecordingObserver
    NoopObserver
    get_observer_state_dict
    load_observer_state_dict
    default_observer
    default_placeholder_observer
    default_debug_observer
    default_weight_observer
    default_histogram_observer
    default_per_channel_weight_observer
    default_dynamic_quant_observer
    default_float_qparams_observer

torch.quantization.fake_quantize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module implements modules which are used to perform fake quantization
during QAT.

.. currentmodule:: torch.quantization.fake_quantize

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    FakeQuantizeBase
    FakeQuantize
    FixedQParamsFakeQuantize
    FusedMovingAvgObsFakeQuantize
    default_fake_quant
    default_weight_fake_quant
    default_per_channel_weight_fake_quant
    default_histogram_fake_quant
    default_fused_act_fake_quant
    default_fused_wt_fake_quant
    default_fused_per_channel_wt_fake_quant
    disable_fake_quant
    enable_fake_quant
    disable_observer
    enable_observer

torch.quantization.qconfig
~~~~~~~~~~~~~~~~~~~~~~~~~~

This module defines `QConfig` and `QConfigDynamic` objects which are used
to configure quantization settings for individual ops.

.. currentmodule:: torch.quantization.qconfig

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    QConfig
    QConfigDynamic
    default_qconfig
    default_debug_qconfig
    default_per_channel_qconfig
    default_dynamic_qconfig
    float16_dynamic_qconfig
    float16_static_qconfig
    per_channel_dynamic_qconfig
    float_qparams_weight_only_qconfig
    default_qat_qconfig
    default_weight_only_qconfig
    default_activation_only_qconfig
    default_qat_qconfig_v2

torch.nn.intrinsic
~~~~~~~~~~~~~~~~~~

This module implements the combined (fused) modules conv + relu which can
then be quantized.

.. currentmodule:: torch.nn.intrinsic

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    ConvReLU1d
    ConvReLU2d
    ConvReLU3d
    LinearReLU
    ConvBn1d
    ConvBn2d
    ConvBn3d
    ConvBnReLU1d
    ConvBnReLU2d
    ConvBnReLU3d
    BNReLU2d
    BNReLU3d

torch.nn.intrinsic.qat
~~~~~~~~~~~~~~~~~~~~~~

This module implements the versions of those fused operations needed for
quantization aware training.

.. currentmodule:: torch.nn.intrinsic.qat

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    LinearReLU
    ConvBn1d
    ConvBnReLU1d
    ConvBn2d
    ConvBnReLU2d
    ConvReLU2d
    ConvBn3d
    ConvBnReLU3d
    ConvReLU3d
    update_bn_stats
    freeze_bn_stats

torch.nn.intrinsic.quantized
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module implements the quantized implementations of fused operations
like conv + relu. No BatchNorm variants as it's usually folded into convolution
for inference.

.. currentmodule:: torch.nn.intrinsic.quantized

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    BNReLU2d
    BNReLU3d
    ConvReLU1d
    ConvReLU2d
    ConvReLU3d
    LinearReLU

torch.nn.intrinsic.quantized.dynamic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module implements the quantized dynamic implementations of fused operations
like linear + relu.

.. currentmodule:: torch.nn.intrinsic.quantized.dynamic

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    LinearReLU

torch.nn.qat
~~~~~~~~~~~~~~~~~~~~~~

This module implements versions of the key nn modules **Conv2d()** and
**Linear()** which run in FP32 but with rounding applied to simulate the
effect of INT8 quantization.

.. currentmodule:: torch.nn.qat

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    Conv2d
    Conv3d
    Linear

torch.nn.quantized
~~~~~~~~~~~~~~~~~~~~~~

This module implements the quantized versions of the nn layers such as
~`torch.nn.Conv2d` and `torch.nn.ReLU`.

.. currentmodule:: torch.nn.quantized

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    ReLU6
    Hardswish
    ELU
    LeakyReLU
    Sigmoid
    BatchNorm2d
    BatchNorm3d
    Conv1d
    Conv2d
    Conv3d
    ConvTranspose1d
    ConvTranspose2d
    ConvTranspose3d
    Embedding
    EmbeddingBag
    FloatFunctional
    FXFloatFunctional
    QFunctional
    Linear
    LayerNorm
    GroupNorm
    InstanceNorm1d
    InstanceNorm2d
    InstanceNorm3d

torch.nn.quantized.functional
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module implements the quantized versions of the functional layers such as
~`torch.nn.functional.conv2d` and `torch.nn.functional.relu`. Note:
:meth:`~torch.nn.functional.relu` supports quantized inputs.

.. currentmodule:: torch.nn.quantized.functional

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    avg_pool2d
    avg_pool3d
    adaptive_avg_pool2d
    adaptive_avg_pool3d
    conv1d
    conv2d
    conv3d
    interpolate
    linear
    max_pool1d
    max_pool2d
    celu
    leaky_relu
    hardtanh
    hardswish
    threshold
    elu
    hardsigmoid
    clamp
    upsample
    upsample_bilinear
    upsample_nearest

torch.nn.quantized.dynamic
~~~~~~~~~~~~~~~~~~~~~~~~~~

Dynamically quantized :class:`~torch.nn.Linear`, :class:`~torch.nn.LSTM`,
:class:`~torch.nn.LSTMCell`, :class:`~torch.nn.GRUCell`, and
:class:`~torch.nn.RNNCell`.

.. currentmodule:: torch.nn.quantized.dynamic

.. autosummary::
    :toctree: generated
    :nosignatures:
    :template: classtemplate.rst

    Linear
    LSTM
    GRU
    RNNCell
    LSTMCell
    GRUCell

Quantized dtypes and quantization schemes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note that operator implementations currently only
support per channel quantization for weights of the **conv** and **linear**
operators. Furthermore the minimum and the maximum of the input data is
mapped linearly to the minimum and the maximum of the quantized data
type such that zero is represented with no quantization error.

Additional data types and quantization schemes can be implemented through
the `custom operator mechanism <https://pytorch.org/tutorials/advanced/torch_script_custom_ops.html>`_.

* :attr:`torch.qscheme` — Type to describe the quantization scheme of a tensor.
  Supported types:

  * :attr:`torch.per_tensor_affine` — per tensor, asymmetric
  * :attr:`torch.per_channel_affine` — per channel, asymmetric
  * :attr:`torch.per_tensor_symmetric` — per tensor, symmetric
  * :attr:`torch.per_channel_symmetric` — per channel, symmetric

* ``torch.dtype`` — Type to describe the data. Supported types:

  * :attr:`torch.quint8` — 8-bit unsigned integer
  * :attr:`torch.qint8` — 8-bit signed integer
  * :attr:`torch.qint32` — 32-bit signed integer
