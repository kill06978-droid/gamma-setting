#include <Geode/Geode.hpp>
#include <Geode/modify/CCEGLView.hpp>

using namespace geode::prelude;

// ---- 설정 캐시 ----
static bool  g_enabled    = true;
static float g_gamma      = 1.f;
static float g_brightness = 0.f;
static float g_contrast   = 1.f;

$execute {
	g_enabled    = Mod::get()->getSettingValue<bool>("enabled");
	g_gamma      = static_cast<float>(Mod::get()->getSettingValue<double>("gamma"));
	g_brightness = static_cast<float>(Mod::get()->getSettingValue<double>("brightness"));
	g_contrast   = static_cast<float>(Mod::get()->getSettingValue<double>("contrast"));

	listenForSettingChanges<bool>("enabled", [](bool v) { g_enabled = v; });
	listenForSettingChanges<double>("gamma", [](double v) { g_gamma = static_cast<float>(v); });
	listenForSettingChanges<double>("brightness", [](double v) { g_brightness = static_cast<float>(v); });
	listenForSettingChanges<double>("contrast", [](double v) { g_contrast = static_cast<float>(v); });
}

// ---- 셰이더 ----
static const char* kVert = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
varying vec2 v_uv;
void main() {
	gl_Position = a_position;
	v_uv = a_texCoord;
}
)";

static const char* kFrag = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_uv;
uniform sampler2D u_tex;
uniform float u_invGamma;
uniform float u_brightness;
uniform float u_contrast;
void main() {
	vec3 c = texture2D(u_tex, v_uv).rgb;
	c = (c - 0.5) * u_contrast + 0.5 + u_brightness;
	c = pow(clamp(c, 0.0, 1.0), vec3(u_invGamma));
	gl_FragColor = vec4(c, 1.0);
}
)";

struct GammaState {
	CCGLProgram* program = nullptr;
	GLuint tex = 0;
	GLint locTex = -1, locGamma = -1, locBright = -1, locContrast = -1;
	GLint texW = 0, texH = 0;
	bool failed = false;
};
static GammaState g_state;

static bool initProgram() {
	auto& s = g_state;
	s.program = new CCGLProgram();
	if (!s.program->initWithVertexShaderByteArray(kVert, kFrag)) {
		log::error("감마 셰이더 컴파일 실패");
		return false;
	}
	s.program->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
	s.program->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
	if (!s.program->link()) {
		log::error("감마 셰이더 링크 실패");
		return false;
	}
	s.program->updateUniforms();

	GLuint id = s.program->getProgram();
	s.locTex      = glGetUniformLocation(id, "u_tex");
	s.locGamma    = glGetUniformLocation(id, "u_invGamma");
	s.locBright   = glGetUniformLocation(id, "u_brightness");
	s.locContrast = glGetUniformLocation(id, "u_contrast");

	glGenTextures(1, &s.tex);
	glBindTexture(GL_TEXTURE_2D, s.tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return true;
}

static void applyGamma() {
	auto& s = g_state;
	if (s.failed) return;

	if (!s.program) {
		if (!initProgram()) { s.failed = true; return; }
	}

	// 현재 뷰포트 영역을 텍스처로 복사
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	if (vp[2] <= 0 || vp[3] <= 0) return;

	glBindTexture(GL_TEXTURE_2D, s.tex);
	if (vp[2] != s.texW || vp[3] != s.texH) {
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vp[0], vp[1], vp[2], vp[3], 0);
		s.texW = vp[2];
		s.texH = vp[3];
	} else {
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp[0], vp[1], vp[2], vp[3]);
	}

	// 전체 화면 쿼드 그리기
	static const GLfloat pos[] = { -1,-1,  1,-1,  -1,1,  1,1 };
	static const GLfloat uv[]  = {  0, 0,  1, 0,   0,1,  1,1 };

	ccGLUseProgram(s.program->getProgram());
	ccGLBindTexture2D(s.tex);
	ccGLBlendFunc(GL_ONE, GL_ZERO);
	ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_TexCoords);

	glUniform1i(s.locTex, 0);
	glUniform1f(s.locGamma, 1.f / std::max(g_gamma, 0.01f));
	glUniform1f(s.locBright, g_brightness);
	glUniform1f(s.locContrast, g_contrast);

	glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, pos);
	glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, uv);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// 원래 블렌드 상태 복구
	ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

class $modify(GammaView, CCEGLView) {
	void swapBuffers() {
		bool identity = std::abs(g_gamma - 1.f) < 0.001f
			&& std::abs(g_brightness) < 0.001f
			&& std::abs(g_contrast - 1.f) < 0.001f;

		if (g_enabled && !identity) {
			applyGamma();
		}
		CCEGLView::swapBuffers();
	}
};
