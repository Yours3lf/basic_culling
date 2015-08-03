#include "intersection.h"
#include "framework.h"

using namespace prototyper;

int main( int argc, char** argv )
{
  shape::set_up_intersection();

  map<string, string> args;

  for( int c = 1; c < argc; ++c )
  {
    args[argv[c]] = c + 1 < argc ? argv[c + 1] : "";
    ++c;
  }

  cout << "Arguments: " << endl;
  for_each( args.begin(), args.end(), []( pair<string, string> p )
  {
    cout << p.first << " " << p.second << endl;
  } );

  uvec2 screen( 0 );
  bool fullscreen = false;
  bool silent = false;
  string title = "Culling poc";

  /*
  * Process program arguments
  */

  stringstream ss;
  ss.str( args["--screenx"] );
  ss >> screen.x;
  ss.clear();
  ss.str( args["--screeny"] );
  ss >> screen.y;
  ss.clear();

  if( screen.x == 0 )
  {
    screen.x = 1280;
  }

  if( screen.y == 0 )
  {
    screen.y = 720;
  }

  try
  {
    args.at( "--fullscreen" );
    fullscreen = true;
  }
  catch( ... ) {}

  try
  {
    args.at( "--help" );
    cout << title << ", written by Marton Tamas." << endl <<
      "Usage: --silent      //don't display FPS info in the terminal" << endl <<
      "       --screenx num //set screen width (default:1280)" << endl <<
      "       --screeny num //set screen height (default:720)" << endl <<
      "       --fullscreen  //set fullscreen, windowed by default" << endl <<
      "       --help        //display this information" << endl;
    return 0;
  }
  catch( ... ) {}

  try
  {
    args.at( "--silent" );
    silent = true;
  }
  catch( ... ) {}

  /*
  * Initialize the OpenGL context
  */

  framework frm;
  frm.init( screen, title, fullscreen );

  //set opengl settings
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LEQUAL );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  glClearDepth( 1.0f );

  frm.get_opengl_error();

  /*
  * Set up mymath
  */

  camera<float> cam;
  frame<float> the_frame;


  float cam_fov = 58.7f;
  float cam_near = 1.0f;
  float cam_far = 1000.0f;

  /**
  //small camera
  cam_fov = 20.0f;
  cam_far = 10.0f;
  /**/

  the_frame.set_perspective( radians( cam_fov ), ( float )screen.x / ( float )screen.y, cam_near, cam_far );

  frame<float> top_frame;
  camera<float> cam_top;
  top_frame.set_perspective( radians( 58.7f ), ( float )screen.x / ( float )screen.y, 1, 100 );
  cam_top.rotate_x( radians( -90.0f ) );
  cam_top.move_forward( -70.0f );

  glViewport( 0, 0, screen.x, screen.y );

  /*
  * Set up the scene
  */

  float move_amount = 5;
  float cam_rotation_amount = 5.0;

  //set up boxes
  GLuint box = frm.create_box();

  int size = 100; //number of boxes to render
  cout << "Rendering: " << size* size << " cubes" << endl;

  vector<mat4> positions;
  positions.resize( size * size );

  for( int c = 0; c < size; ++c )
  {
    for( int d = 0; d < size; ++d )
    {
      positions[c * size + d] = mm::create_translation( mm::vec3( c * 30 - size, -20, -d * 30 ) ) * mm::create_scale(mm::vec3(10));
    }
  }

  /*
  * Set up the shaders
  */

  GLuint debug_shader = 0;
  frm.load_shader( debug_shader, GL_VERTEX_SHADER, "../shaders/culling/debug.vs" );
  frm.load_shader( debug_shader, GL_FRAGMENT_SHADER, "../shaders/culling/debug.ps" );

  GLint debug_proj_mat_loc = glGetUniformLocation( debug_shader, "proj" );
  GLint debug_view_mat_loc = glGetUniformLocation( debug_shader, "view" );
  GLint debug_model_mat_loc = glGetUniformLocation( debug_shader, "model" );
  GLint debug_light_loc = glGetUniformLocation( debug_shader, "light" );

  /*
  * Set up culling
  */

  bool cull = true;

  /**/
  //calculate camera and box bounding volumes
  vec4 box_center = vec4( vec3( 0 ), 1 );
  float box_radius = length( vec3( 2, 2, 2 ) ) * 0.5f;
  /**/

  vector<shape*> bvs;
  bvs.resize( size * size );

  for( int c = 0; c < size; ++c )
  {
    for( int d = 0; d < size; ++d )
    {
      vec4 model_box_center = positions[c * size + d] * box_center;

      //represent boxes as aabbs or spheres
      //spheres are a tiny bit faster
      bvs[c * size + d] = new aabb( model_box_center.xyz, vec3( 10 ) );
      //bvs[c * size + d] = new sphere( model_box_center.xyz, box_radius );
    }
  }

  shape* cam_sphere = new sphere( vec3(0), float(0) );

  shape* cam_frustum = new frustum();

  /*
  * Handle events
  */

  //allows to switch between the topdown and normal camera
  camera<float>* cam_ptr = &cam;

  bool warped = false, ignore = true;
  vec2 movement_speed = vec2(0);

  auto event_handler = [&]( const sf::Event & ev )
  {
    switch( ev.type )
    {
    case sf::Event::MouseMoved:
      {
        vec2 mpos( ev.mouseMove.x / float( screen.x ), ev.mouseMove.y / float( screen.y ) );

        if( warped )
        {
          ignore = false;
        }
        else
        {
          frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
          warped = true;
          ignore = true;
        }

        if( !ignore && all( notEqual( mpos, vec2( 0.5 ) ) ) )
        {
          cam.rotate( radians( -180.0f * ( mpos.x - 0.5f ) ), vec3( 0.0f, 1.0f, 0.0f ) );
          cam.rotate_x( radians( -180.0f * ( mpos.y - 0.5f ) ) );
          frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
          warped = true;
        }

        break;
      }
    case sf::Event::KeyPressed:
      {/*
       if( ev.key.code == sf::Keyboard::A )
       {
       if( cam_ptr == &cam )
       cam_ptr->rotate_y( radians( cam_rotation_amount ) );
       else if( cam_ptr == &cam_top )
       cam_ptr->rotate_z( radians( -cam_rotation_amount ) );
       }

       if( ev.key.code == sf::Keyboard::D )
       {
       if( cam_ptr == &cam )
       cam_ptr->rotate_y( radians( -cam_rotation_amount ) );
       else if( cam_ptr == &cam_top )
       cam_ptr->rotate_z( radians( cam_rotation_amount ) );
       }

       if( ev.key.code == sf::Keyboard::W )
       {
       if( cam_ptr == &cam )
       cam_ptr->move_forward( move_amount );
       else if( cam_ptr == &cam_top )
       cam_ptr->move_up( move_amount );
       }

       if( ev.key.code == sf::Keyboard::S )
       {
       if( cam_ptr == &cam )
       cam_ptr->move_forward( -move_amount );
       else if( cam_ptr == &cam_top )
       cam_ptr->move_up( -move_amount );
       }*/

        if( ev.key.code == sf::Keyboard::Space )
        {
          cull = !cull;
        }

        if( ev.key.code == sf::Keyboard::C )
        {
          cam_ptr = &cam;
        }

        if( ev.key.code == sf::Keyboard::X )
        {
          cam_ptr = &cam_top;
        }

        break;
      }
    default:
      break;
    }
  };

  /*
  * Render
  */

  sf::Clock timer;
  timer.restart();

  ss.clear();
  int draw_count = 0;
  int intersection_count = 0;
  int frame_count = 0;

  sf::Clock movement_timer;
  movement_timer.restart();

  frm.display( [&]
  {
    frm.handle_events( event_handler );

    float seconds = movement_timer.getElapsedTime().asMilliseconds() / 1000.0f;

    if( seconds > 0.01667 )
    {
      //move camera
      if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) )
      {
        movement_speed.x -= move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) )
      {
        movement_speed.x += move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) )
      {
        movement_speed.y += move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) )
      {
        movement_speed.y -= move_amount;
      }

      cam.move_forward( movement_speed.y * seconds );
      cam.move_right( movement_speed.x * seconds );
      movement_speed *= 0.5;

      movement_timer.restart();
    }

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( debug_shader );

    glUniformMatrix4fv( debug_proj_mat_loc, 1, false, &the_frame.projection_matrix[0][0] );

    mat4 view = cam_ptr->get_matrix();
    glUniformMatrix4fv( debug_view_mat_loc, 1, false, &view[0][0] );

    glBindVertexArray( box );

    //set up camera
    //need to set up per change (per frame for now)
    static_cast<frustum*>( cam_frustum )->set_up( cam, the_frame );

    vec3 cam_center = cam.pos + cam.view_dir * ( ( cam_far - cam_near ) * 0.5f ) + cam_near;
    float cam_radius = length(
      //far top right apex
      ( ( cam.pos - cam.view_dir* the_frame.far_ll.z ) + cam.up_vector * ( the_frame.far_ul.y - the_frame.far_ll.y ) -
      -normalize( cross( cam.up_vector, cam.view_dir ) ) * ( the_frame.far_lr.x - the_frame.far_ll.x ) )

      - cam_center ); //center of the circle around the camera

    static_cast<sphere*>( cam_sphere )->set_center( cam_center );
    static_cast<sphere*>( cam_sphere )->set_radius( cam_radius );

    //contains the IDs and state (culled or not) of the cubes
    static vector< pair< int, bool > > in_view;

    for( int c = 0; c < size; ++c )
    {
      for( int d = 0; d < size; ++d )
      {
        bool culled = false;

        if( cull )
        {
          /**
          //camera is approximated by a sphere
          if( !culled )
          {
            ++intersection_count;

            if( !shape::intersects.go( cam_sphere, bvs[c * size + d] ) )
              culled = true;
          }

          /**/

          /**/
          //camera is a frustum
          //precise intersection
          if( !culled )
          {
            intersection_count += 6;

            if( !cam_frustum->is_intersecting( bvs[c * size + d] ) )
              culled = true;
          }

          /**/
        }

        /**/
        //comment out to see coloring
        if( culled )
          continue;

        /**/

        in_view.push_back( make_pair( c * size + d, culled ) );
      }
    }

    for( auto c = in_view.begin(); c != in_view.end(); ++c )
    {
      vec3 red( 1, 0, 0 );
      vec3 green( 0, 1, 0 );

      if( c->second )
        glUniform3fv( debug_light_loc, 1, &red.x );
      else
        glUniform3fv( debug_light_loc, 1, &green.x );

      glUniformMatrix4fv( debug_model_mat_loc, 1, false, &positions[c->first][0][0] );
      glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
      ++draw_count;
    }

    in_view.clear();

    /**/
    if( cam_ptr == &cam_top )
    {
      //draw camera
      vec3 blue( 1, 1, 1 );

      glUniform3fv( debug_light_loc, 1, &blue.x );

      mat4 m = mat4::identity;

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_CULL_FACE);
      glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );

      vec3 cam_base = cam.pos;

      //m = create_translation( cam.pos + vec3( 0, 1, 0 ) );
      //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
      //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

      glBegin(GL_TRIANGLES);

      float nw = the_frame.near_lr.x - the_frame.near_ll.x;
      float nh = the_frame.near_ul.y - the_frame.near_ll.y;

      float fw = the_frame.far_lr.x - the_frame.far_ll.x;
      float fh = the_frame.far_ul.y - the_frame.far_ll.y;

      vec3 nc = cam.pos - cam.view_dir * the_frame.near_ll.z;
      vec3 fc = cam.pos - cam.view_dir * the_frame.far_ll.z;

      vec3 right = -normalize( cross( cam.up_vector, cam.view_dir ) );

      //near top left
      vec3 ntl = nc + cam.up_vector * nh - right * nw;
      vec3 ntr = nc + cam.up_vector * nh + right * nw;
      vec3 nbl = nc - cam.up_vector * nh - right * nw;
      vec3 nbr = nc - cam.up_vector * nh + right * nw;

      vec3 ftl = fc + cam.up_vector * fh - right * fw;
      vec3 ftr = fc + cam.up_vector * fh + right * fw;
      vec3 fbl = fc - cam.up_vector * fh - right * fw;
      vec3 fbr = fc - cam.up_vector * fh + right * fw;

      glVertex3fv((GLfloat*)&cam_base);
      glVertex3fv((GLfloat*)&ftl);
      glVertex3fv((GLfloat*)&fbl);

      glVertex3fv((GLfloat*)&cam_base);
      glVertex3fv((GLfloat*)&ftr);
      glVertex3fv((GLfloat*)&fbr);

      glVertex3fv((GLfloat*)&cam_base);
      glVertex3fv((GLfloat*)&ftl);
      glVertex3fv((GLfloat*)&ftr);

      glVertex3fv((GLfloat*)&cam_base);
      glVertex3fv((GLfloat*)&fbl);
      glVertex3fv((GLfloat*)&fbr);

      //far plane
      //m = create_translation( vec3( ntl.x, 1, ntl.z ) );
      //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
      //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

      //m = create_translation( vec3( ntr.x, 1, ntr.z ) );
      //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
      //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

      //near plane
      //m = create_translation( vec3( ftl.x, 1, ftl.z ) );
      //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
      //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

      //m = create_translation( vec3( ftr.x, 1, ftr.z ) );
      //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
      //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

      glEnd();

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_CULL_FACE);
    }
    /**/

    ++frame_count;

    if( timer.getElapsedTime().asMilliseconds() > 1000 && !silent )
    {
      int timepassed = timer.getElapsedTime().asMilliseconds();
      int fps = 1000.0f / ( ( float ) timepassed / ( float ) frame_count );

      string ttl = title;

      ss << " - FPS: " << fps
        << " - Time: " << ( float ) timepassed / ( float ) frame_count
        << " - Draw calls: " << ( draw_count / frame_count )
        << " - Intersections: " << ( intersection_count / frame_count );

      ttl += ss.str();
      ss.str( "" );

      frm.set_title( ttl );

      intersection_count = 0;
      draw_count = 0;
      frame_count = 0;
      timer.restart();
    }

    frm.get_opengl_error();
  }, silent );

  return 0;
}
