import operator

import torch
import torch.fx
from torch.testing._internal.common_fx2trt import VanillaTestCase


class TestAddConverter(VanillaTestCase):
    def test_operator_add(self):
        def add(x):
            return x + x

        inputs = [torch.randn(1, 1)]
        self.run_test(add, inputs, expected_ops={operator.add})

    def test_torch_add(self):
        def add(x):
            return torch.add(x, x)

        inputs = [torch.randn(1, 1)]
        self.run_test(add, inputs, expected_ops={torch.add})
