attribute vec4 vPosition;

//set up to pass color to fshader.
attribute vec4 vColor;
varying vec4 fColor;

attribute vec4 vNormal;
uniform vec4 LightPosition;
uniform float Shininess;
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;

uniform mat4 p;    //projection view matrix
uniform mat4 mv;    //model view matrix
uniform mat4 RD;


uniform float scale;  //the scale 
uniform vec2 pos;  //the location of circle 

void main()
{

   vec4 ambient, diffuse, specular;
   vec3 L,E;
	
   //these have to be in column major order.
   mat4 S = mat4(scale, 0.0, 0.0, 0.0,
                 0.0, scale, 0.0, 0.0, 
                 0.0, 0.0, 1.0, 0.0, 
                 0.0, 0.0, 0.0, 1.0);
   mat4 T = mat4(1.0, 0.0, 0.0, 0.0,
                 0.0, 1.0, 0.0, 0.0, 
                 0.0, 0.0, 1.0, 0.0, 
                 pos.x, pos.y, 0.0, 1.0);
   gl_Position =  p *  mv * RD*vPosition;
   
   vec3 N = normalize(vNormal.xyz);
   
   L = normalize(LightPosition.xyz - (mv*vPosition).xyz);
   	
   E = -normalize((mv*vPosition).xyz);
   
   vec3 H = normalize(L+E);
   
   float Kd = max(dot(L, N), 0.0);
   float Ks = pow(max(dot(N, H), 0.0), Shininess);
   
   ambient = vColor;
   diffuse = Kd*DiffuseProduct;
   specular = max(pow(max(dot(N, H), 0.0), Shininess)*SpecularProduct, 0.0);
   
   //if(LightPosition.z >=-7)
   	fColor = vec4((ambient + diffuse + specular).xyz, 1.0);
  // else
	//fColor = vColor;

}
