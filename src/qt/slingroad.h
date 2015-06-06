#ifndef SLINGROAD_H
#define SLINGROAD_H

#include <QWidget>
#include <QTimer>
#include <QListWidgetItem>

namespace Ui {
    class SlingRoad;
}



class SlingRoad : public QWidget
{
    Q_OBJECT

public:
    explicit SlingRoad(QWidget *parent = 0);
    ~SlingRoad();
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    void updateCategories();
    double lastPrice;

public slots:
    void updateCategory(QString category);

private:
    Ui::SlingRoad *ui;

private slots:
    void on_tableWidget_itemSelectionChanged();
    void on_categoriesListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void on_buyButton_clicked();
    void on_viewDetailsButton_clicked();
    void on_copyAddressButton_clicked();
};

#endif // SLINGROAD_H
