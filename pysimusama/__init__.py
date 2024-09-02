import bpy
import gpu
from gpu_extras.batch import batch_for_shader
import numpy as np
from . import mpm_simulation

bl_info = {
    "name": "MPM Simulation",
    "author": "Your Name",
    "version": (1, 4),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > MPM Simulation",
    "description": "Run MPM simulations in Blender with multiple materials",
    "category": "Animation",
}

vertex_shader = '''
    uniform mat4 viewProjectionMatrix;

    in vec3 position;

    void main()
    {
        gl_Position = viewProjectionMatrix * vec4(position, 1.0);
    }
'''

fragment_shader = '''
    uniform vec4 color;

    out vec4 fragColor;

    void main()
    {
        fragColor = color;
    }
'''

class MPMSimulationOperator(bpy.types.Operator):
    bl_idname = "object.mpm_simulation"
    bl_label = "Run MPM Simulation"

    _timer = None
    _draw_handle = None
    simulation = None
    particles = None
    current_frame = 0
    shader = None
    batch = None

    def execute(self, context):
        if context.area.type != 'VIEW_3D':
            self.report({'WARNING'}, "View3D not found, cannot run operator")
            return {'CANCELLED'}

        # Initialize simulation
        grid_size = 100
        dt = context.scene.render.fps_base / context.scene.render.fps
        self.simulation = mpm_simulation.Simulation(grid_size, dt)

        # Set simulation attributes
        self.simulation.set_attr("gravity", -9.81)
        self.simulation.set_attr("rest_density", 1000.0)
        self.simulation.set_attr("dynamic_viscosity", 0.001)
        self.simulation.set_attr("particle_mass", 0.1)

        # Add particles
        num_particles = 1000
        particles = np.random.rand(num_particles, 3) * 0.5
        self.simulation.add_particles(particles.astype(np.float64), 0)

        # Initialize shader
        self.shader = gpu.types.GPUShader(vertex_shader, fragment_shader)

        # Set up modal
        context.window_manager.modal_handler_add(self)
        self._timer = context.window_manager.event_timer_add(dt, window=context.window)

        # Set up draw handler
        self._draw_handle = bpy.types.SpaceView3D.draw_handler_add(
            self.draw_sim, (context,), 'WINDOW', 'POST_VIEW'
        )

        self.current_frame = context.scene.frame_start

        return {'RUNNING_MODAL'}

    def modal(self, context, event):
        if event.type == 'ESC':
            self.cancel(context)
            return {'CANCELLED'}

        if event.type == 'TIMER':
            self.simulation.simulate(1)
            self.current_frame += 1
            context.area.tag_redraw()

            print("PROCESS FRAME %i" % self.current_frame)

            if self.current_frame > context.scene.frame_end:
                self.cancel(context)
                return {'FINISHED'}

        return {'PASS_THROUGH'}

    def cancel(self, context):
        bpy.types.SpaceView3D.draw_handler_remove(self._draw_handle, 'WINDOW')
        context.window_manager.event_timer_remove(self._timer)

    def draw_sim(self, context):
        particles = list(self.simulation.particles)
        if particles:
            coords = [(p.x, p.y, p.z) for p in particles]
            self.batch = batch_for_shader(self.shader, 'POINTS', {"position": coords})

            self.shader.bind()

            matrix = context.region_data.perspective_matrix
            self.shader.uniform_float("viewProjectionMatrix", matrix)
            self.shader.uniform_float("color", (1, 0, 0, 1))  # Red color for particles

            gpu.state.point_size_set(5)  # Set point size
            gpu.state.blend_set('ALPHA')  # Enable alpha blending

            self.batch.draw(self.shader)

            gpu.state.blend_set('NONE')  # Disable alpha blending after drawing

class MPMSimulationPanel(bpy.types.Panel):
    bl_label = "MPM Simulation"
    bl_idname = "OBJECT_PT_mpm_simulation"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "MPM Simulation"

    def draw(self, context):
        layout = self.layout
        layout.operator("object.mpm_simulation")

def register():
    bpy.utils.register_class(MPMSimulationOperator)
    bpy.utils.register_class(MPMSimulationPanel)

def unregister():
    bpy.utils.unregister_class(MPMSimulationOperator)
    bpy.utils.unregister_class(MPMSimulationPanel)

if __name__ == "__main__":
    register()