#include <QApplication>
#include <QMenu>
#include <QPoint> 
#include <QPushButton>
#include <QHBoxLayout> 
#include <QVBoxLayout> 
#include <QMouseEvent>
#include <dbusmenuimporter.h>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  a.setQuitOnLastWindowClosed(true);

  QString service(argv[1]);
  QString path(argv[2]);

  DBusMenuImporter importMenu(service, path);
  QMenu *menu = importMenu.menu();

  QWidget window;

  QVBoxLayout *vbox_layout = new QVBoxLayout(&window);
  //QHBoxLayout *hbox_layout = new QHBoxLayout(vbox_layout);
  window.setLayout(vbox_layout);
  QWidget *empty = new QWidget(&window);
  empty->setAttribute(Qt::WA_NoSystemBackground, true);
  empty->setAttribute(Qt::WA_TranslucentBackground, true);
  empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  vbox_layout->addWidget(empty);

  QPushButton *button = new QPushButton("⚙");
  vbox_layout->addWidget(button);
  button->setMenu(menu);
  //button->showMenu();
  //window.setWindowState(Qt::WindowFullScreen);
  window.setWindowState(Qt::WindowMaximized);
  //window.setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
  window.setParent(0); // Create TopLevel-Widget
  window.setAttribute(Qt::WA_NoSystemBackground, true);
  window.setAttribute(Qt::WA_TranslucentBackground, true);
  window.show();

//QMouseEvent event(QEvent::MouseButtonPress, QPointF(0,0), QPoint(0,0), Qt::LeftButton, Qt::AllButtons, Qt::NoModifier);
//QApplication::postEvent(button, &event);

  return a.exec();
}
