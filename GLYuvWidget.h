#ifndef GLYUVWIDGET_H
#define GLYUVWIDGET_H

#include <QOpenGLWidget>
#include <YUV420P_Render.h>
// #include <QOpenGLFunctions>
// #include <QOpenGLBuffer>
// #include <QTimer>

// QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
// QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class GLYuvWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    GLYuvWidget(QWidget *parent =0);
    ~GLYuvWidget();

public slots:
    void slotShowYuv(uchar *ptr,uint width,uint height); //显示一帧Yuv图像

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;

private:
    // QOpenGLShaderProgram *program;
    // QOpenGLBuffer vbo;
    // GLuint textureUniformY,textureUniformU,textureUniformV; //opengl中y、u、v分量位置
    // QOpenGLTexture *textureY = nullptr,*textureU = nullptr,*textureV = nullptr;
    // GLuint idY,idU,idV; //自己创建的纹理对象ID，创建错误返回0
    YUV420P_Render *m_pYUV420P_Render;
    uint videoW,videoH;
    uchar *yuvPtr = nullptr;
};

#endif // GLYUVWIDGET_H
