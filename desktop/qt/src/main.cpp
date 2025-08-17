\
    #include <QApplication>
    #include <QMainWindow>
    #include <QVBoxLayout>
    #include <QHBoxLayout>
    #include <QPushButton>
    #include <QLineEdit>
    #include <QPlainTextEdit>
    #include <QFileDialog>
    #include <QProcess>
    #include <QElapsedTimer>
    #include <QLabel>

    class MainWin : public QMainWindow {
      Q_OBJECT
    public:
      MainWin() {
        auto *central = new QWidget;
        auto *v = new QVBoxLayout;
        auto *h = new QHBoxLayout;
        open = new QPushButton("Open");
        run = new QPushButton("Run");
        path = new QLineEdit;
        out = new QPlainTextEdit;
        out->setReadOnly(true);
        status = new QLabel("Ready");

        h->addWidget(open);
        h->addWidget(path, 1);
        h->addWidget(run);
        v->addLayout(h);
        v->addWidget(out, 1);
        v->addWidget(status);
        central->setLayout(v);
        setCentralWidget(central);
        resize(900, 600);
        setWindowTitle("Vitte Desktop (Qt)");

        connect(open, &QPushButton::clicked, this, &MainWin::onOpen);
        connect(run, &QPushButton::clicked, this, &MainWin::onRun);
      }
    private slots:
      void onOpen() {
        auto f = QFileDialog::getOpenFileName(this, "Open .vitte", QString(), "Vitte (*.vitte *.vit);;All (*)");
        if(!f.isEmpty()) path->setText(f);
      }
      void onRun() {
        if(path->text().isEmpty()){ append("⚠ Select a file first."); return; }
        QElapsedTimer t; t.start();
        QProcess p;
        p.start("vitte", {"run", path->text()});
        p.waitForFinished(-1);
        append(QString::fromUtf8(p.readAllStandardOutput()));
        append(QString::fromUtf8(p.readAllStandardError()));
        status->setText(QString("✔ Done in %1 ms, code %2").arg(t.elapsed()).arg(p.exitCode()));
      }
    private:
      void append(const QString &s){ if(!s.isEmpty()) out->appendPlainText(s.trimmed()); }
      QPushButton *open; QPushButton *run; QLineEdit *path; QPlainTextEdit *out; QLabel *status;
    };

    #include "main.moc"

    int main(int argc, char **argv){
      QApplication app(argc, argv);
      MainWin w; w.show();
      return app.exec();
    }
