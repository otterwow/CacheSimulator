#include "precomp.h"
// preliminaries
#define FRAMES 32		// 32 for buddhabrot debugging, 4096 for brot final, 256 for leaf
#define _oOo_oOo_ ][O>=V|N>=V?0:O*V+N
uint* image[4], I,N,F,O,M,_O,V=2018; double K[999], Q[999];
void Game::Init(){for(V=800,F=1,I=1;I<4;I++)image[I]=(uint*)calloc(V*V,4);}
float R(){I^=I<<13;I^=I>>17;I^=I<<5;return I*2.3283064365387e-10f*6-3;} // rng
timer tm;
void Game::Tick( float d )
{
	tm.reset();
#if 0
	double // based on www.rosettacode.org		ACCESS PATTERN: SOMEWHAT STRUCTURED
	z,q,x=0,y=0;for(F=0;++F<9e5;){N=775-y*
	45,z=R(),O=x*55+400;if(z<.14){if(z<.1)
	{if(z<.01)q=0,y/=5;else q=x/5-y/3,y=(x
	+y+8)/5;}else q=y/3-.1*x,y=.5+x/3+y/5;		// data access; to be cached:
	}else q=.9*x+.04*y,y=.9*y-.04*x+2;x=q;		STOREUINT(&image[1 _oOo_oOo_ ],R()*14+80);
												STOREUINT(&image[2 _oOo_oOo_ ],R()*40+168);
	/* END OF BLACK BOX CODE */					STOREUINT(&image[3 _oOo_oOo_ ],R()*13+30);}
#else
	if(F<FRAMES)   // based on Paul Bourke		ACCESS PATTERN: MOSTLY RANDOM
	for(int G,M,T,E=0;++E<4;)for(G=0;++G<V
	<<7;){double B=0,y=0,t=R(),e,z=R();for
	(T=0;T<E<<8;){e=2*B*y+z,B=K[T]=B*B-y*y
	+t,y=Q[T++]=e;if(B*B+y*y>9){for(M=0;M<		// data access; to be cached:
	T;){O=400+.3*V*Q[M],N=.3*V*K[M++]+520;		STOREUINT(&image[E _oOo_oOo_ ],
												 LOADUINT(&image[E _oOo_oOo_ ])+1)
	/* END OF BLACK BOX CODE */;}break;}}}
#endif

	// visualize data - rotated 90 degrees, gamma corrected
	for( int x = 0; x < 800; x++ ) for( int y = 0; y < 800; y++ )
	{
		uint cr = min( 255, (uint)( 255.0f * sqrtf( LOADUINT( &image[1][y + x * 800] ) * (1.0f / FRAMES) ) ) );
		uint cg = min( 255, (uint)( 255.0f * sqrtf( LOADUINT( &image[2][y + x * 800] ) * (1.0f / FRAMES) ) ) );
		uint cb = min( 255, (uint)( 255.0f * sqrtf( LOADUINT( &image[3][y + x * 800] ) * (1.0f / FRAMES) ) ) );
		STOREUINT( screen->GetBuffer() + x + y * 800, cr + (cg << 8) + (cb << 16 ) );
	}

	// report cache performance
	auto observer = GetObserver();
#if DRAWPERF
	observer->Draw(screen, 100u);
#endif
	observer->Print(F, tm.elapsed());
	observer->Update();

	// report success
	if (++F == FRAMES) printf( "done.\n" );
}