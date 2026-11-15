#include <dcmtk/dcmjpeg/djdecode.h>
#include <dcmtk/dcmjpls/djdecode.h>

#include "window.hpp"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    DJDecoderRegistration::registerCodecs();
    DJLSDecoderRegistration::registerCodecs();

    MainWindow test;
    test.show();

    app.exec();
}