import os
import sys
import tempfile
import types
import unittest


class FakeModel3D:
    def __init__(self, fail_on=None):
        self.history = []
        self.fail_on = fail_on

    def add_to_history(self, name, command):
        if isinstance(self.fail_on, (list, tuple, set)):
            if any(item in command for item in self.fail_on):
                raise RuntimeError("No plot data available for export")
        elif self.fail_on and self.fail_on in command:
            raise RuntimeError("No plot data available for export")
        self.history.append((name, command))


def install_fake_cst_module():
    fake_cst = types.ModuleType("cst")
    fake_interface = types.ModuleType("cst.interface")

    class FakeDesignEnvironment:
        def new_mws(self):
            return types.SimpleNamespace(model3d=FakeModel3D())

    fake_interface.DesignEnvironment = FakeDesignEnvironment
    fake_cst.interface = fake_interface

    sys.modules["cst"] = fake_cst
    sys.modules["cst.interface"] = fake_interface


install_fake_cst_module()

import cst_controller


class ExportFarfieldTests(unittest.TestCase):
    def test_export_farfield_uses_ascii_export_object(self):
        controller = cst_controller.CSTController.__new__(cst_controller.CSTController)
        controller.project = types.SimpleNamespace(model3d=FakeModel3D())

        controller.export_farfield(r"C:\temp\patch245_farfield.txt", 2.45)

        history_text = "\n".join(command for _, command in controller.project.model3d.history)

        self.assertIn('With ASCIIExport', history_text)
        self.assertIn('.FileName "C:/temp/patch245_farfield.txt"', history_text)
        self.assertIn('.Execute', history_text)
        self.assertNotIn('With FarfieldPlot', history_text)

    def test_export_farfield_tries_alternative_result_names(self):
        controller = cst_controller.CSTController.__new__(cst_controller.CSTController)
        controller.project = types.SimpleNamespace(
            model3d=FakeModel3D(
                fail_on=[
                    'Farfields\\farfield (f=2.45) [1]',
                    '1D Results\\Farfields\\farfield (f=2.45) [1]',
                ]
            )
        )

        controller.export_farfield(r"C:\temp\patch245_farfield.txt", 2.45)

        commands = [command for _, command in controller.project.model3d.history]
        self.assertEqual(len(commands), 1)
        self.assertIn('SelectTreeItem "Farfields\\farfield (f=2.45)"', commands[0])

    def test_export_farfield_falls_back_to_result_file(self):
        controller = cst_controller.CSTController.__new__(cst_controller.CSTController)
        controller.project = types.SimpleNamespace(model3d=FakeModel3D(fail_on='Farfields'))

        with tempfile.TemporaryDirectory() as tmp_dir:
            project_name = os.path.join(tmp_dir, 'patch245.cst')
            project_folder = os.path.join(tmp_dir, 'patch245')
            result_dir = os.path.join(project_folder, 'Result')
            os.makedirs(result_dir)
            result_file = os.path.join(result_dir, 'farfield (f=2.45)_1.ffm')
            with open(result_file, 'w', encoding='utf-8') as fh:
                fh.write('ffm fallback data')

            controller.project_path = project_name

            output_file = os.path.join(tmp_dir, 'patch245_farfield.ffm')
            controller.export_farfield(output_file, 2.45)

            self.assertTrue(os.path.exists(output_file))
            with open(output_file, 'r', encoding='utf-8') as fh:
                self.assertIn('ffm fallback data', fh.read())


if __name__ == "__main__":
    unittest.main()
