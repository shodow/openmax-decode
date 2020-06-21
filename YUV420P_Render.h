#ifndef YUV420P_RENDER_H
#define YUV420P_RENDER_H

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QTime>
#include <QGLShaderProgram>

class YUV420P_Render: protected QOpenGLFunctions
{

public:
    YUV420P_Render();
    ~YUV420P_Render();

    //初始化gl
    void initialize();
    //刷新显示
    void render(uchar* ptr,int width,int height,int type);

private:
    //shader程序
    QOpenGLShaderProgram m_program;
    //shader中yuv变量地址
    GLuint m_textureUniformY, m_textureUniformU , m_textureUniformV;
    //创建纹理
    GLuint m_idy , m_idu , m_idv;

    QTime time;
    bool m_first;
};

#endif // YUV420P_RENDER_H
