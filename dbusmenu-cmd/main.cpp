#include <QApplication>
#include <QMenu>
#include <QPoint> 
#include <QPushButton>
#include <dbusmenuimporter.h>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  a.setQuitOnLastWindowClosed(true);

  QString service(argv[1]);
  QString path(argv[2]);

  QPushButton button("⚙");
  
  DBusMenuImporter importMenu(service, path);
  QMenu *menu = importMenu.menu();

  button.setMenu(menu);
  button.show();

  return a.exec();
}
