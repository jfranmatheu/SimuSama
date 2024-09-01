import bpy
import bmesh
import numpy as np
from . import mpm_simulation

bl_info = {
    "name": "MPM Simulation",
    "author": "Your Name",
    "version": (1, 3),
    "blender": (2, 80, 0),
    "location": "View3D > Sidebar > MPM Simulation",
    "description": "Run MPM simulations in Blender with multiple materials",
    "category": "Animation",
}


sim = None
 
class MPMSimulationProperties(bpy.types.PropertyGroup):
    grid_size_x: bpy.props.IntProperty(name="Grid Size X", default=50, min=10, max=200)
    grid_size_y: bpy.props.IntProperty(name="Grid Size Y", default=50, min=10, max=200)
    grid_size_z: bpy.props.IntProperty(name="Grid Size Z", default=50, min=10, max=200)
    cell_size: bpy.props.FloatProperty(name="Cell Size", default=0.1, min=0.01, max=1.0)
    time_step: bpy.props.FloatProperty(name="Time Step", default=0.01, min=0.001, max=0.1)
    num_steps: bpy.props.IntProperty(name="Number of Steps", default=100, min=1, max=1000)
    particle_scale: bpy.props.FloatProperty(name="Particle Scale", default=0.05, min=0.01, max=1.0)
    material_type: bpy.props.EnumProperty(
        name="Material Type",
        items=[
            ('0', "Elastic", "Elastic material"),
            ('1', "Snow", "Snow material"),
            ('2', "Fluid", "Fluid material"),
        ],
        default='0'
    )

class MPMSimulationPanel(bpy.types.Panel):
    bl_label = "MPM Simulation"
    bl_idname = "OBJECT_PT_mpm_simulation"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "MPM Simulation"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        mpm_props = scene.mpm_properties

        layout.prop(mpm_props, "grid_size_x")
        layout.prop(mpm_props, "grid_size_y")
        layout.prop(mpm_props, "grid_size_z")
        layout.prop(mpm_props, "cell_size")
        layout.prop(mpm_props, "time_step")
        layout.prop(mpm_props, "num_steps")
        layout.prop(mpm_props, "particle_scale")
        layout.prop(mpm_props, "material_type")
        layout.operator("object.run_mpm_simulation")

class RunMPMSimulation(bpy.types.Operator):
    bl_idname = "object.run_mpm_simulation"
    bl_label = "Run MPM Simulation"

    def execute(self, context):
        scene = context.scene
        mpm_props = scene.mpm_properties

        # Create MPM simulation
        global sim
        grid_dimensions = np.array([mpm_props.grid_size_x, mpm_props.grid_size_y, mpm_props.grid_size_z])
        sim = mpm_simulation.MPMSimulation(grid_dimensions, mpm_props.cell_size, mpm_props.time_step)

        # Add particles from selected object
        obj = context.active_object
        if obj and obj.type == 'MESH':
            bm = bmesh.new()
            bm.from_mesh(obj.data)
            bmesh.ops.triangulate(bm, faces=bm.faces)

            material_index = int(mpm_props.material_type)
            for v in bm.verts:
                world_pos = obj.matrix_world @ v.co
                sim.add_particle(np.array(world_pos), 1.0, material_index)

            bm.free()
        else:
            self.report({'ERROR'}, "Please select a mesh object")
            return {'CANCELLED'}

        # Create result mesh
        mesh = bpy.data.meshes.new(name="MPM_Result")
        result_obj = bpy.data.objects.new("MPM_Result", mesh)
        bpy.context.scene.collection.objects.link(result_obj)

        # Set up frame range
        scene.frame_start = 0
        scene.frame_end = mpm_props.num_steps - 1

        # Add frame_post handler
        bpy.app.handlers.frame_change_post.append(update_mpm_simulation)
        bpy.ops.screen.animation_play('INVOKE_DEFAULT')

        # particle_count = sim.get_particle_count()
        # self.report({'INFO'}, f"MPM Simulation setup complete with {particle_count} particles")
        return {'FINISHED'}

def update_mpm_simulation(scene):
    print(f"MPM Simulating frame {scene.frame_current}")

    global sim
    result_obj = bpy.data.objects["MPM_Result"]

    try:
        # Run simulation step
        sim.step()

        # Update mesh
        particles = sim.get_particles()
        
        if not particles:
            print("Warning: No particles returned from simulation")
            return

        mesh = result_obj.data
        mesh.clear_geometry()
        mesh.vertices.add(len(particles))

        coords = []
        for particle in particles:
            try:
                coords.extend(particle.position)
            except AttributeError:
                print(f"Error: Particle has no position attribute")
            except Exception as e:
                print(f"Error accessing particle position: {str(e)}")

        if len(coords) == len(particles) * 3:
            mesh.vertices.foreach_set("co", coords)
        else:
            print(f"Error: Mismatch in coordinate data. Expected {len(particles) * 3}, got {len(coords)}")

        # Update mesh
        mesh.update()
        
        print(f"Updated mesh with {len(particles)} particles")
    except Exception as e:
        print(f"Error in MPM simulation update: {str(e)}")



def register():
    bpy.utils.register_class(MPMSimulationProperties)
    bpy.utils.register_class(MPMSimulationPanel)
    bpy.utils.register_class(RunMPMSimulation)
    bpy.types.Scene.mpm_properties = bpy.props.PointerProperty(type=MPMSimulationProperties)

def unregister():
    bpy.utils.unregister_class(MPMSimulationProperties)
    bpy.utils.unregister_class(MPMSimulationPanel)
    bpy.utils.unregister_class(RunMPMSimulation)
    del bpy.types.Scene.mpm_properties
    bpy.app.handlers.frame_change_post.remove(update_mpm_simulation)

if __name__ == "__main__":
    register()