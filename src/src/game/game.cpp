#include "game.h"
#include "../graph/menudialog.h"
#include "enemyai.h"
#include "graphview.h"
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QString>
#include <QThread>
#include <QWindow>


Game::~Game() {
    clearConnection();
    clearMemory();
}

void Game::clearConnection() {
    for(auto item : connection) {
        disconnect(item);
    }
    connection.clear();
}

void Game::clearMemory() {
    for(int i = 1; i <= width(); i++) {
        for(int j = 1; j <= height(); j++) {
            delete m_blocks[i][j];
        }
    }
    for(int i = 0; i < m_units.size(); i++) {
        delete m_units[i];
    }
    m_graph->deleteLater();
    m_field->deleteLater();
    enemy->deleteLater();
    m_blocks.clear();
    m_units.clear();
}

Game::Game(const QString &filename, QWidget *parent)
    : Game(
          [](const QString &filename) -> QJsonObject {
              QJsonObject json;
              openJson(filename, json);
              return json;
          }(filename),
          parent) {}

Game::Game(const qint32 &level, QWidget *parent)
    : Game(
          [](qint32 level) -> QString {
              return "json/level" + QString::number(level) + ".json";
          }(level),
          parent) {}

Game::Game(const QJsonObject &json, QWidget *parent)
    : GraphView(parent), policyTreeButtonWidget(nullptr) {
    read(json);
}

void Game::read(const QJsonObject &json) {
    // 本质上是一个 read 函数
    if(json.contains("gameInfo") && json["gameInfo"].isObject()) {
        m_gameInfo.read(json["gameInfo"].toObject());
    }
    if(json.contains("unitTypeInfo") && json["unitTypeInfo"].isObject()) {
        m_unitTypeInfo.clear();
        QJsonObject unitTypeInfo = json["unitTypeInfo"].toObject();
        for(auto it = unitTypeInfo.begin(); it != unitTypeInfo.end(); it++) {
            QJsonObject unitInfo = it.value().toObject();
            m_unitTypeInfo[it.key().toInt()].read(unitInfo);
        }
    }
    if(json.contains("blockTypeInfo") && json["blockTypeInfo"].isObject()) {
        m_blockTypeInfo.clear();
        QJsonObject blockTypeInfo = json["blockTypeInfo"].toObject();
        for(auto it = blockTypeInfo.begin(); it != blockTypeInfo.end(); it++) {
            QJsonObject blockInfo = it.value().toObject();
            m_blockTypeInfo[it.key().toInt()].read(blockInfo);
        }
    }
    if(json.contains("blocks") && json["blocks"].isArray()) {
        QJsonArray blocks = json["blocks"].toArray();
        int        cnt    = 0;
        m_blocks.clear();
        m_blocks.resize(width() + 2);
        for(int i = 1; i <= width(); i++) {
            m_blocks[i].resize(height() + 2);
            for(int j = 1; j <= height(); j++) {
                m_blocks[i][j] = new BlockStatus(blocks[cnt].toObject(), this);
                m_blocks[i][j]->m_info =
                    &m_blockTypeInfo[m_blocks[i][j]->m_type];
                cnt++;
            }
        }
    }
    if(json.contains("units") && json["units"].isArray()) {
        QJsonArray units = json["units"].toArray();
        m_units.clear();
        m_units.resize(units.size());
        for(int i = 0; i < units.size(); i++) {
            m_units[i]         = new UnitStatus(units[i].toObject(), this);
            m_units[i]->m_info = &m_unitTypeInfo[m_units[i]->m_type];
        }
    }
    m_field = new Field(m_gameInfo, m_blocks, m_units, this);
    m_graph = new GraphField(m_gameInfo, m_blocks, m_units, this);
    enemy   = new EnemyAI(this, 2, this);
}

void Game::write(QJsonObject &json) {
    QJsonObject gameInfo;
    m_gameInfo.write(gameInfo);
    json["gameInfo"] = gameInfo;
    {
        QJsonObject unitTypeInfo;
        for(auto it = m_unitTypeInfo.begin(); it != m_unitTypeInfo.end();
            it++) {
            QJsonObject unitInfo;
            it.value().write(unitInfo);
            unitTypeInfo[QString::number(it.key())] = unitInfo;
        }
        json["unitTypeInfo"] = unitTypeInfo;
    }
    {
        QJsonObject blockTypeInfo;
        for(auto it = m_blockTypeInfo.begin(); it != m_blockTypeInfo.end();
            it++) {
            QJsonObject blockInfo;
            it.value().write(blockInfo);
            blockTypeInfo[QString::number(it.key())] = blockInfo;
        }
        json["blockTypeInfo"] = blockTypeInfo;
    }
    {
        QJsonArray blocks;
        for(int i = 1; i <= width(); i++) {
            for(int j = 1; j <= height(); j++) {
                QJsonObject block;
                m_blocks[i][j]->write(block);
                blocks.append(block);
            }
        }
        json["blocks"] = blocks;
    }
    {
        QJsonArray units;
        for(int i = 0; i < m_units.size(); i++) {
            QJsonObject unit;
            m_units[i]->write(unit);
            units.append(unit);
        }
        json["units"] = units;
    }
}

void Game::setgameStatusLabel() {
    // 应该重载一下那个 Label 的，不过之后再说吧，现在先写一个能用的
    QLabel *gameStatusLabel = new QLabel();
    gameStatusLabel->setStyleSheet("font: 15pt ;");
    connection.append(connect(this, &Game::gameStatusUpdated, this, [=]() {
        this->updateGameStatus(gameStatusLabel);
    }));
    gameStatusLabelWidget = m_graph->addWidget(gameStatusLabel);
    gameStatusLabelWidget->setFlag(QGraphicsItem::ItemIgnoresTransformations,
                                   true);
    this->updateGameStatus(gameStatusLabel);
}

void Game::updateGameStatus(QLabel *gameStatusLabel) {
    gameStatusLabel->setText(
        "  || 回合数：" + QString::number(m_gameInfo.m_turnNumber) +
        " || 我方存留 " + QString::number(m_gameInfo.m_campNumbers[0]) +
        " || 敌方存留 " + QString::number(m_gameInfo.m_campNumbers[1]) +
        " ||  ");
}

void Game::setFixedWidgetPos() {
    if(m_graph->views().length() == 0)
        return;
    GraphView *m_view = dynamic_cast<GraphView *>(m_graph->views().first());
    if(m_view == nullptr)
        return;
    if(nextTurnButtonWidget != nullptr) {
        nextTurnButtonWidget->setPos(m_view->mapToScene(
            QPoint(m_view->size().width(), m_view->height()) -
            QPoint(nextTurnButtonWidget->size().width(),
                   nextTurnButtonWidget->size().height())));
        nextTurnButtonWidget->setZValue(GraphInfo::buttonZValue);
    }
    if(newUnitButtonWidget != nullptr) {
        newUnitButtonWidget->setPos(m_view->mapToScene(
            QPoint(m_view->size().width() / 2 -
                       newUnitButtonWidget->size().width() / 2,
                   m_view->height() - newUnitButtonWidget->size().height())));
        newUnitButtonWidget->setZValue(GraphInfo::buttonZValue);
    }
    if(pauseButtonWidget != nullptr) {
        pauseButtonWidget->setPos(
            m_view->mapToScene(QPoint(m_view->size().width() - 100, 0)));
        pauseButtonWidget->setZValue(GraphInfo::buttonZValue);
    }
    if(policyTreeButtonWidget != nullptr) {
        policyTreeButtonWidget->setPos(
            m_view->mapToScene(QPoint(0, m_view->size().height() - 100)));
        policyTreeButtonWidget->setZValue(GraphInfo::buttonZValue);
    }
    if(gameStatusLabelWidget != nullptr) {
        gameStatusLabelWidget->setPos(m_view->mapToScene(
            QPoint(m_view->size().width() / 2 -
                       gameStatusLabelWidget->size().width() / 2,
                   0)));
        gameStatusLabelWidget->setZValue(GraphInfo::buttonZValue);
    }
}

void Game::setView() {
    this->setScene(m_graph);
    connection.append(connect(this, &GraphView::finishPainting, this,
                              &Game::setFixedWidgetPos));
}

void Game::init() {
    setNewUnitButton();
    setNextTurnButton();
    setPolicyTreeButton();
    setPauseButton();
    setgameStatusLabel();
    setView();
    connection.append(
        connect(m_graph, &GraphField::checkStateChange, this,
                [=](QPoint coord, bool state) {
                    if(state == true &&
                       canNewUnitAt(m_gameInfo.nowPlayer, blocks(coord))) {
                        showNewUnitButton();
                    }
                    else {
                        hideNewUnitButton();
                    }
                }));

    // connection.append(connect(m_graph, &GraphField::userNewUnit,m_field,
    // &Field::doNewUnit);
    connection.append(
        connect(m_graph, &GraphField::userMoveUnit, m_field,
                QOverload<qint32, QPoint>::of(&Field::doUnitMove)));
    connection.append(
        connect(m_graph, &GraphField::userAttackUnit, m_field,
                QOverload<qint32, QPoint>::of(&Field::doUnitAttack)));
    connection.append(connect(m_graph, &GraphField::userShowMoveRange, m_field,
                              &Field::getUnitMoveRange));

    connection.append(
        connect(m_field, &Field::newUnit, m_graph, &GraphField::newUnit));
    connection.append(
        connect(m_field, &Field::unitDead, m_graph, &GraphField::dieUnit));
    connection.append(
        connect(m_field, &Field::campDead, this, [=](QPoint coord) {
            if(blocks(coord)->m_type == peopleCampBlock) {
                m_gameInfo.m_campNumbers[0]--;
            }
            else if(blocks(coord)->m_type == virusCampBlock) {
                m_gameInfo.m_campNumbers[1]--;
            }
            else {
                Q_ASSERT(0);
            }
            emit gameStatusUpdated();
            if(m_gameInfo.m_campNumbers[0] <= 0) {
                emit lose(1);
            }
            if(m_gameInfo.m_campNumbers[1] <= 0) {
                emit lose(2);
            }
        }));
    connection.append(connect(
        m_field, &Field::moveUnit, m_graph,
        QOverload<qint32, const QVector<QPoint> &>::of(&GraphField::moveUnit)));
    connection.append(
        connect(m_field, &Field::attackUnit, m_graph,
                QOverload<qint32, qint32, QPair<qreal, qreal>>::of(
                    &GraphField::attackUnit)));
    connection.append(
        connect(m_field, &Field::attackCamp, m_graph, &GraphField::attackCamp));
    connection.append(connect(m_field, &Field::unitMoveRangegot, m_graph,
                              &GraphField::showMoveRange));

    connection.append(connect(m_graph, &GraphField::userHideMoveRange, m_graph,
                              &GraphField::hideMoveRange));
}

void Game::setDetailedLabel(QLabel *detailedLabel) {
    connection.append(
        connect(m_graph, &GraphField::checkStateChange, this,
                [=]() { this->updateDetailedStatus(detailedLabel); }));
    // connection.append(
    //     connect(m_graph, &GraphField::needUpdateDetail, this,
    //             [=]() { this->updateDetailedStatus(detailedLabel); }));

    this->updateDetailedStatus(detailedLabel);
}

void Game::updateDetailedStatus(QLabel *detailedLabel) {
    if(m_graph->m_nowCheckedBlock == nullptr) {
        detailedLabel->setText("当前无选中格。");
    }
    else {
        QString text = "当前选中格为：(" +
            QString::number(m_graph->m_nowCheckedBlock->coord().x()) + "," +
            QString::number(m_graph->m_nowCheckedBlock->coord().y()) + ")\n";
        switch(m_graph->m_nowCheckedBlock->m_status->m_type) {
            case plainBlock:
                text.append("当前地形为平原，可以生产 Unit。\n");
                break;
            case obstacleBlock:
                text.append("当前地形为障碍，不能生产、通过 Unit。\n");
                break;
            default:
                text.append("是的，这是一个bug。\n");
                break;
        }
        if(m_graph->m_nowCheckedBlock->unitOnBlock() == -1) {
            text.append("当前格上无单元。");
        }
        else {
            text.append(
                "当前格上单元编号：" +
                QString::number(m_graph->m_nowCheckedBlock->unitOnBlock()) +
                "。\n");
            UnitStatus *unitStatus =
                m_units[m_graph->m_nowCheckedBlock->unitOnBlock()];
            text.append("血量：" + QString::number(unitStatus->getHP()) +
                        " 移动力：" + QString::number(unitStatus->getMP()) +
                        " 攻击力：" + QString::number(unitStatus->getCE()));
        }
        detailedLabel->setText(text);
    }
}

void Game::setNextTurnButton() {
    QPushButton *nextTurnButton = new QPushButton();
    nextTurnButton->setIcon(QIcon(":/icons/nextturn.svg"));
    nextTurnButton->setWhatsThis("当前回合完成，进入下一回合");

    nextTurnButton->setIconSize(QSize(85, 85));
    nextTurnButton->setContentsMargins(5, 5, 10, 10);
    nextTurnButtonWidget = m_graph->addWidget(nextTurnButton);
    nextTurnButtonWidget->setFlag(QGraphicsItem::ItemIgnoresTransformations,
                                  true);

    nextTurnButtonWidget->setGeometry(QRect(QPoint(0, 0), QPoint(100, 100)));
    connection.append(connect(nextTurnButton, &QPushButton::clicked, this,
                              &Game::usernextTurn));
    emit gameStatusUpdated();
}

void Game::usernextTurn() {
    QJsonObject json;
    write(json);
    // writeJson("3.json", json);
restart:
    if(m_gameInfo.playerNumbers == m_gameInfo.nowPlayer) {
        m_gameInfo.nowPlayer = 1;
        m_gameInfo.m_turnNumber++;
    }
    else {
        m_gameInfo.nowPlayer += 1;
    }
    for(int i = 0; i < m_units.size(); i++) {
        if(m_units[i]->m_player == m_gameInfo.nowPlayer) {
            m_units[i]->setAttackState(true);
            m_units[i]->setMoveState(true);
        }
        else {
            m_units[i]->setAttackState(false);
            m_units[i]->setMoveState(false);
        }
    }
    if(m_gameInfo.nowPlayer == enemy->m_player) {
        enemy->play();
        goto restart;
    }
    QMessageBox msgBox;
    msgBox.setText("进入下一玩家游戏。当前是第 " +
                   QString::number(m_gameInfo.m_turnNumber) + " 回合，第 " +
                   QString::number(m_gameInfo.nowPlayer) +
                   " 号玩家，请开始操控。");
    msgBox.exec();

    emit gameStatusUpdated();
}

void Game::setPauseButton() {
    QPushButton *pauseButton = new QPushButton();
    pauseButton->setGeometry(-100, 0, 100, 100);
    pauseButton->setIcon(QIcon(":/icons/pause.png"));
    pauseButton->setIconSize(QSize(85, 85));
    pauseButton->setContentsMargins(5, 5, 10, 10);
    pauseButtonWidget = m_graph->addWidget(pauseButton);
    pauseButtonWidget->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    connection.append(
        connect(pauseButton, &QPushButton::clicked, this, &Game::userPause));
}

void Game::userPause() {
    QString          fileName;
    PauseMenuDialog *window = new PauseMenuDialog(this, fileName, this);
    int              t      = window->exec();
    if(t == 1) {
        // theoritically 应该退出
    }
    else if(t == 2) {
        emit loadGame(fileName);
        // 读档
    }
    window->deleteLater();
    // window->show();
    // m_graph->addWidget(window);
}

void Game::setNewUnitButton() {
    QPushButton *newUnitButton = new QPushButton();
    newUnitButton->setText("新建兵");
    newUnitButton->setWhatsThis("在选中的Block上新建一个兵吧！");
    newUnitButtonWidget = m_graph->addWidget(newUnitButton);
    newUnitButtonWidget->setGeometry(QRect(QPoint(0, 0), QPoint(100, 100)));
    newUnitButtonWidget->setFlag(QGraphicsItem::ItemIgnoresTransformations,
                                 true);
    // 初始时需要隐藏
    newUnitButtonWidget->hide();

    connection.append(
        connect(newUnitButton, &QPushButton::clicked, this, [=]() {
            NewUnitDialog *newunit     = new NewUnitDialog(this, this);
            int            newUnitType = newunit->exec();
            delete newunit;
            if(newUnitType == 0)
                return;
            Q_ASSERT(m_graph->m_nowCheckedBlock != nullptr);
            this->usernewUnit(m_graph->m_nowCheckedBlock->coord(),
                              UnitType(newUnitType));
            emit m_graph->checkStateChange(m_graph->m_nowCheckedBlock->coord(),
                                           false);
            m_graph->m_nowCheckedBlock = nullptr;
        }));
}

void Game::usernewUnit(QPoint coord, UnitType newUnitType) {
    // 需要当前位置没有Unit，否则会炸掉的
    BlockStatus *block = blocks(coord);
    Q_ASSERT(block != nullptr);
    Q_ASSERT(block->m_unitOnBlock == -1);
    UnitStatus *unitStatus = new UnitStatus(m_units.size(), newUnitType,
                                            &m_unitTypeInfo[newUnitType],
                                            m_gameInfo.nowPlayer, coord, this);
    m_units.push_back(unitStatus);
    unitStatus->setAttackState(false);
    unitStatus->setMoveState(false);

    m_field->doNewUnit(unitStatus);
}

void Game::setPolicyTreeButton() {
    // 暂时不加了
    QPushButton *policyTreeButton = new QPushButton();
    policyTreeButton->setGeometry(QRect(0, -100, 100, 100));
    policyTreeButton->setIcon(QIcon(":/icons/policytree.png"));
    policyTreeButton->setIconSize(QSize(85, 85));
    policyTreeButton->setContentsMargins(5, 5, 5, 5);
    // policyTreeButtonWidget = m_graph->addWidget(policyTreeButton);
    // policyTreeButtonWidget->setFlag(QGraphicsItem::ItemIgnoresTransformations,
    // true);
    connection.append(connect(policyTreeButton, &QPushButton::clicked, this,
                              &Game::usershowPolicyTree));
}

void Game::usershowPolicyTree() {
    PolicyTreeDialog t(this, nullptr);
    t.exec();
}
