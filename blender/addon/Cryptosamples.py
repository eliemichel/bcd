"""
/!\ This add-on requires a custom version of Blender to support cryptosamples output.

Usage: Select a render layer node, press space, look for "Cryptosamples Output"

License
-------

Copyright (c) 2017-2018 Elie Michel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

bl_info = {
    "name": "Cryptosamples Output",
    "author": "Elie Michel",
    "version": (1, 0),
    "blender": (2, 79, 0),
    "location": "Node Editor",
    "description": "Auto generation of output nodes for sending cryptosamples to BCD cli",
    "warning": "",
    "wiki_url": "",
    "category": "Render",
    }

import bpy
from mathutils import Vector

cryptosamples_hist_bin_layer_tpl = "Histogram Bin%03d"

def main(operator, context):
    space = context.space_data
    node_tree = space.node_tree
    node_active = context.active_node
    
    if node_active is None or node_active.type != 'R_LAYERS':
        operator.report({'ERROR'}, "Active node must be a Render Layers node")
        return

    crl = context.scene.render.layers[node_active.layer].cycles
    if not crl.use_pass_cryptosamples:
        operator.report({'ERROR'}, "The layer {0} has no cryptosamples enabled. Checkout its render layer settings.".format(node_active.layer))
        return
    
    def link_layer_to_output(layer, output_node, output_name):
        output_node.file_slots.new(name=output_name)
        socket_in = output_node.inputs[output_name]
        socket_out = node_active.outputs[layer]
        return node_tree.links.new(socket_in, socket_out)

    # Mean output
    node_output_mean = node_tree.nodes.new(type='CompositorNodeOutputFile')
    node_output_mean.base_path = "//cryptosamples/"
    node_output_mean.format.file_format = 'OPEN_EXR'
    node_output_mean.format.color_depth = '32'
    node_output_mean.file_slots.clear()
    link_layer_to_output("Sample Mean", node_output_mean, "mean_###")
    node_output_mean.location = node_active.location + Vector([2 * node_active.dimensions[0], 0])
    
    # Variance output
    node_output_var = node_tree.nodes.new(type='CompositorNodeOutputFile')
    node_output_var.base_path = "//cryptosamples/variance_###"
    node_output_var.format.file_format = 'OPEN_EXR_MULTILAYER'
    node_output_var.format.color_depth = '32'
    node_output_var.file_slots.clear()
    link_layer_to_output("Sample Variance", node_output_var, "00_Variance")
    link_layer_to_output("Sample Covariance", node_output_var, "01_Covariance")
    node_output_var.location = node_active.location + Vector([2 * node_active.dimensions[0], -150])
    
    # Histogram output
    node_output_hist = node_tree.nodes.new(type='CompositorNodeOutputFile')
    node_output_hist.base_path = "//cryptosamples/histogram_###"
    node_output_hist.format.file_format = 'OPEN_EXR_MULTILAYER'
    node_output_hist.format.color_depth = '32'
    node_output_hist.file_slots.clear()
    for i in range(crl.cryptosamples_hist_bins):
        output_name = cryptosamples_hist_bin_layer_tpl % i
        link_layer_to_output(output_name, node_output_hist, "Bin%03d" % i)
    link_layer_to_output("Sample Count", node_output_hist, "Count")
    node_output_hist.location = node_active.location + Vector([2 * node_active.dimensions[0], -300])

    cmd = "bcd-cli.exe -o cryptosamples/denoised_{0:03d}.exr -i cryptosamples/mean_{0:03d}.exr -bh cryptosamples/histogram_{0:03d}.exr -bc -i cryptosamples/covariance_{0:03d}.exr".format(context.scene.frame_current)
    print("To denoise, run:\n" + cmd + "\n")


class OutputCryptosamplesOp(bpy.types.Operator):
    """Add file output nodes for cryptosample data as expected by the BCD denoiser"""
    bl_idname = "node.output_cryptosamples"
    bl_label = "Output Cryptosamples"

    @classmethod
    def poll(cls, context):
        space = context.space_data
        return space.type == 'NODE_EDITOR'

    def execute(self, context):
        main(self, context)
        return {'FINISHED'}


def register():
    bpy.utils.register_class(OutputCryptosamplesOp)


def unregister():
    bpy.utils.unregister_class(OutputCryptosamplesOp)


if __name__ == "__main__":
    register()
