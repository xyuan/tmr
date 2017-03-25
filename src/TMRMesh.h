#ifndef TMR_MESH_H
#define TMR_MESH_H

#include "TMRBase.h"
#include "TMRGeometry.h"
#include "TMRTopology.h"

/*
  Global options for meshing
*/
class TMRMeshOptions {
 public:
  enum TriangleSmoothingType { LAPLACIAN, SPRING };
  TMRMeshOptions(){
    // Set the default meshing options
    num_smoothing_steps = 10;
    tri_smoothing_type = LAPLACIAN;
    frontal_quality_factor = 1.5;

    // By default, write nothing to any files
    write_init_domain_triangle = 0;
    write_pre_smooth_triangle = 0;
    write_post_smooth_triangle = 0;
    write_dual_recombine = 0;
    write_pre_smooth_quad = 0;
    write_post_smooth_quad = 0;
    write_quad_dual = 0;
  }

  // Options to control the meshing algorithm
  int num_smoothing_steps;
  TriangleSmoothingType tri_smoothing_type;
  double frontal_quality_factor;

  // Write intermediate surface meshes to file
  int write_init_domain_triangle;
  int write_pre_smooth_triangle;
  int write_post_smooth_triangle;
  int write_dual_recombine;
  int write_pre_smooth_quad;
  int write_post_smooth_quad;
  int write_quad_dual;
};

/*
  The mesh for a geometric curve
*/
class TMREdgeMesh : public TMREntity {
 public:
  TMREdgeMesh( MPI_Comm _comm, TMREdge *edge );
  ~TMREdgeMesh();

  // Is this edge mesh degenerate
  int isDegenerate(){ return edge->isDegenerate(); }

  // Retrieve the underlying curve
  void getEdge( TMREdge **_edge );

  // Mesh the geometric object
  void mesh( TMRMeshOptions options, double htarget );

  // Order the mesh points uniquely
  int setNodeNums( int *num );
  int getNodeNums( const int **_vars );

  // Retrieve the mesh points
  void getMeshPoints( int *_npts, const double **_pts, TMRPoint **X );

 private:
  MPI_Comm comm;
  TMREdge *edge;

  // The parametric locations of the points that are obtained from
  // meshing the curve
  int npts; // number of points along the curve
  double *pts; // Parametric node locations
  TMRPoint *X; // Physical node locations
  int *vars; // Global node variable numbers
};

/*
  Triangle info class

  This class is used to generate a triangular mesh
*/
class TMRFaceMesh : public TMREntity {
 public:
  enum TMRFaceMeshType { NO_MESH, STRUCTURED, UNSTRUCTURED };
  TMRFaceMesh( MPI_Comm _comm, TMRFace *face );
  ~TMRFaceMesh();

  // Retrieve the underlying surface
  void getFace( TMRFace **_surface );
  
  // Mesh the underlying geometric object
  void mesh( TMRMeshOptions options, double htarget, 
             TMRFaceMeshType _type=STRUCTURED );

  // Return the type of the underlying mesh
  TMRFaceMeshType getMeshType(){
    return mesh_type;
  }

  // Retrieve the mesh points
  void getMeshPoints( int *_npts, const double **_pts, TMRPoint **X );

  // Order the mesh points uniquely
  int setNodeNums( int *num );
  int getNodeNums( const int **_vars );
  int getNumFixedPoints();

  // Retrieve the local connectivity from this surface mesh
  int getLocalConnectivity( const int **quads );

  // Write the quadrilateral mesh to a VTK format
  void writeToVTK( const char *filename );

  // Print the quadrilateral quality
  void addQuadQuality( int nbins, int count[] );
  void printQuadQuality();

 private:
  // Write the segments to a VTK file in parameter space
  void writeSegmentsToVTK( const char *filename, 
                           int npts, const double *params, 
                           int nsegs, const int segs[] );

  // Print the triangle quality
  void printTriQuality( int ntris, const int tris[] );
  void writeTrisToVTK( const char *filename,
                       int ntris, const int tris[] );
  
  // Write the dual mesh - used for recombination - to a file
  void writeDualToVTK( const char *filename, int nodes_per_elem, 
                       int nelems, const int elems[],
                       int num_dual_edges, const int dual_edges[],
                       const TMRPoint *p );

  // Recombine the mesh to a quadrilateral mesh based on the
  // quad-Blossom algorithm
  void recombine( int ntris, const int tris[], const int tri_neighbors[],
                  const int node_to_tri_ptr[], const int node_to_tris[],
                  int num_edges, const int dual_edges[],
                  int *_num_quads, int **_new_quads,
                  TMRMeshOptions options );

  // Simplify the quadrilateral mesh to remove points that make
  // for a poor quadrilateral mesh
  void simplifyQuads();

  // Compute recombined triangle information
  int getRecombinedQuad( const int tris[], const int trineighbors[],
                         int t1, int t2, int quad[] );
  double computeRecombinedQuality( const int tris[], 
                                   const int trineighbors[],
                                   int t1, int t2, const TMRPoint *p ); 

  // Compute the quadrilateral quality
  double computeQuadQuality( const int *quad, const TMRPoint *p );
  double computeTriQuality( const int *tri, const TMRPoint *p );

  // The underlying surface
  MPI_Comm comm;
  TMRFace *face;

  // The actual type of mesh used to mesh the structure
  TMRFaceMeshType mesh_type;

  // Points in the mesh
  int num_fixed_pts; // number of fixed points
  int num_points; // The number of point locations
  double *pts; // The parametric node locations
  TMRPoint *X; // The physical node locations
  int *vars; // The global variable numbers

  // Quadrilateral mesh information
  int num_quads;
  int *quads;
};

/*
  TMRVolumeMesh class

  This is the class that contains the volume mesh. This mesh 
  takes in the arguments needed to form a volume mesh
*/
class TMRVolumeMesh : public TMREntity {
 public:
  TMRVolumeMesh( MPI_Comm _comm, TMRVolume *volume );
  ~TMRVolumeMesh();
  
  // Create the volume mesh
  int mesh( TMRMeshOptions options );

  // Retrieve the local connectivity from this volume mesh
  int getLocalConnectivity( const int **quads );

  // Order the mesh points uniquely
  int setNodeNums( int *num );
  int getNodeNums( const int **_vars );

  // Write the volume mesh to a VTK file
  void writeToVTK( const char *filename );

 private:
  // The underlying volume
  MPI_Comm comm;
  TMRVolume *volume;

  // Set the number of face loops
  int num_face_loops;
  int *face_loop_ptr;
  TMRFace **face_loops;
  int *face_loop_edge_count;

  // Number of points through-thickness
  int num_depth_pts;

  // Keep the bottom/top surfaces (master/slave) in the
  // mesh for future reference
  TMRFace *bottom, *top;

  int num_points; // The number of points
  TMRPoint *X; // The physical node locations
  int *vars; // The global variable numbers

  // Hexahedral mesh information
  int num_hexes;
  int *hexes;
};

/*
  Mesh the geometry model.

  This class handles the meshing for surface objects without any
  additional information. For hexahedral meshes, the model must
*/
class TMRMesh : public TMREntity {
 public:
  TMRMesh( MPI_Comm _comm, TMRModel *_geo );
  ~TMRMesh();

  // Mesh the underlying geometry
  void mesh( double htarget );
  void mesh( TMRMeshOptions options, double htarget );

  // Write the mesh to a VTK file
  void writeToVTK( const char *filename );

  // Write the mesh to a BDF file
  void writeToBDF( const char *filename );

  // Retrieve the mesh components
  int getMeshPoints( TMRPoint **_X );
  int getMeshConnectivity( const int **_quads );

  // Create a topology object (with underlying mesh geometry)
  TMRModel* createModelFromMesh();

 private:
  // Allocate and initialize the underlying mesh
  void initMesh();

  // The underlying geometry object
  MPI_Comm comm;
  TMRModel *geo;

  // The number of nodes/positions in the mesh
  int num_nodes;
  TMRPoint *X;  

  // The number of quads
  int num_quads;
  int *quads;

  // The number of hexes
  int num_hexes;
  int *hexes;
};

#endif // TMR_MESH_H
