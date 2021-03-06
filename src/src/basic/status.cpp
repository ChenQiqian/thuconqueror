#include "status.h"
#include <QJsonArray>

bool openJsonAbsPath(const QString &filename, QJsonObject &json) {
    //打开文件
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "File open failed!";
    }
    else {
        qDebug() << "File open successfully!";
    }
    QJsonParseError *error = new QJsonParseError;
    QJsonDocument    jdc   = QJsonDocument::fromJson(file.readAll(), error);

    //判断文件是否完整
    if(error->error != QJsonParseError::NoError) {
        qDebug() << "parseJson:" << error->errorString();
        return false;
    }

    json = jdc.object();  //获取对象
    return true;
}

bool openJson(const QString &filename, QJsonObject &json) {
    //打开文件
    QFile file(QApplication::applicationDirPath() + "/" + filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "File open failed!";
    }
    else {
        qDebug() << "File open successfully!";
    }
    QJsonParseError *error = new QJsonParseError;
    QJsonDocument    jdc   = QJsonDocument::fromJson(file.readAll(), error);

    //判断文件是否完整
    if(error->error != QJsonParseError::NoError) {
        qDebug() << "parseJson:" << error->errorString();
        return false;
    }

    json = jdc.object();  //获取对象
    return true;
}

bool writeJsonAbsPath(const QString &filename, const QJsonObject &json) {
    //打开文件
    QFile file("/" + filename);
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "File open failed!";
        return false;
    }
    else {
        qDebug() << "File open successfully!";
    }
    QJsonDocument jdoc;
    jdoc.setObject(json);
    file.write(
        jdoc.toJson(QJsonDocument::Compact));  // Indented:表示自动添加/n回车符
    file.close();
    return false;
}

bool writeJson(const QString &filename, const QJsonObject &json) {
    //打开文件
    QFile file(QApplication::applicationDirPath() + "/" + filename);
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "File open failed!";
        return false;
    }
    else {
        qDebug() << "File open successfully!";
    }
    QJsonDocument jdoc;
    jdoc.setObject(json);
    file.write(
        jdoc.toJson(QJsonDocument::Compact));  // Indented:表示自动添加/n回车符
    file.close();
    return false;
}

void GameInfo::read(const QJsonObject &json) {
    if(json.contains("map_size") && json["map_size"].isArray())
        map_size = QPoint{json["map_size"].toArray()[0].toInt(),
                          json["map_size"].toArray()[1].toInt()};
    if(json.contains("nowPlayer") && json["nowPlayer"].isDouble())
        nowPlayer = UnitType(json["nowPlayer"].toInt());
    if(json.contains("turnNumber") && json["turnNumber"].isDouble())
        m_turnNumber = json["turnNumber"].toInt();
    if(json.contains("playerNumbers") && json["playerNumbers"].isDouble())
        playerNumbers = json["playerNumbers"].toInt();
    if(json.contains("level") && json["level"].isDouble())
        level = json["level"].toInt();
    if(json.contains("speed") && json["speed"].isDouble())
        speed = json["speed"].toInt();

    if(json.contains("campNumbers") && json["campNumbers"].isArray()) {
        m_campNumbers.clear();
        QJsonArray arr = json["campNumbers"].toArray();
        for(auto num : arr) {
            if(num.isDouble()) {
                m_campNumbers.append(num.toInt());
            }
        }
    }
}

void GameInfo::write(QJsonObject &json) {
    QJsonArray size;
    size.append(map_size.x());
    size.append(map_size.y());
    json["map_size"]      = size;
    json["nowPlayer"]     = nowPlayer;
    json["turnNumber"]    = m_turnNumber;
    json["playerNumbers"] = playerNumbers;
    json["speed"]         = speed;
    QJsonArray arr;
    for(auto num : m_campNumbers)
        arr.push_back(num);
    json["campNumbers"] = arr;
    json["level"]       = level;
}

void BlockStatus::write(QJsonObject &json) {
    json["type"] = m_type;
    QJsonArray coord;
    coord.append(m_coord.x());
    coord.append(m_coord.y());
    json["coord"]       = coord;
    json["unitOnBlock"] = m_unitOnBlock;
    json["HPnow"]       = m_HPnow;
}
void BlockStatus::read(const QJsonObject &json) {
    if(json.contains("type") && json["type"].isDouble())
        m_type = BlockType(json["type"].toInt());
    if(json.contains("coord") && json["coord"].isArray())
        m_coord = QPoint{json["coord"].toArray()[0].toInt(),
                         json["coord"].toArray()[1].toInt()};
    if(json.contains("unitOnBlock") && json["unitOnBlock"].isDouble())
        m_unitOnBlock = json["unitOnBlock"].toInt();
    if(json.contains("HPnow") && json["HPnow"].isDouble())
        m_HPnow = json["HPnow"].toDouble();
}

bool BlockStatus::changeHP(qreal delta) {
    Q_ASSERT(m_info->HPfull != 0);
    m_HPnow += delta / (m_info->HPfull);
    return m_HPnow < 0;
}

UnitStatus::UnitStatus(QJsonObject json, QObject *parent) : QObject(parent){
    this->read(json);
}

UnitStatus::UnitStatus(const int &uid, const UnitType type, UnitInfo *uInfo,
                       qint32 player, QPoint coord ,QObject * parent)
    : QObject(parent), m_uid(uid), m_info(uInfo), m_type(type), m_player(player),
      m_nowCoord(coord), m_canMove(true), m_canAttack(true), m_HPnow(1) {}


bool UnitStatus::changeHP(qreal delta) {
    Q_ASSERT(m_info->HPfull != 0);
    Q_ASSERT(m_info->HPratio != 0);
    // 预备了一个技能树调整的“比例”，没有用的机会
    m_HPnow += delta / (m_info->HPfull * m_info->HPratio);
    return !isAlive();
}

void UnitStatus::read(const QJsonObject &json) {
    if(json.contains("uid") && json["uid"].isDouble())
        m_uid = json["uid"].toInt();
    if(json.contains("type") && json["type"].isDouble())
        m_type = UnitType(json["type"].toInt());
    if(json.contains("player") && json["player"].isDouble())
        m_player = json["player"].toInt();
    if(json.contains("nowCoord") && json["nowCoord"].isArray())
        m_nowCoord = QPoint{json["nowCoord"].toArray()[0].toInt(),
                            json["nowCoord"].toArray()[1].toInt()};
    if(json.contains("HPnow") && json["HPnow"].isDouble())
        m_HPnow = json["HPnow"].toDouble();
    if(json.contains("canMove") && json["canMove"].isBool())
        m_canMove = json["canMove"].toBool();
    if(json.contains("canAttack") && json["canAttack"].isBool())
        m_canAttack = json["canAttack"].toBool();
}

void UnitStatus::write(QJsonObject &json) const {
    json["uid"] = m_uid;
    json["type"]   = m_type;
    json["player"] = m_player;
    QJsonArray nowCoord;
    nowCoord.append(m_nowCoord.x());
    nowCoord.append(m_nowCoord.y());
    json["nowCoord"]  = nowCoord;
    json["HPnow"]     = m_HPnow;
    json["canMove"]   = m_canMove;
    json["canAttack"] = m_canAttack;
}

void UnitStatus::setMoveState(bool state) {
    m_canMove = state;
    emit unitStateChanged();
}
void UnitStatus::setAttackState(bool state) {
    m_canAttack = state;
    emit unitStateChanged();
}

QPair<qreal, qreal> calculateAttack(UnitStatus *source, UnitStatus *target) {
    qreal a = source->getCE(), b = target->getCE();
    return qMakePair(b, a);
}

// 手绘周围的点，需要分奇偶讨论
QPoint nearby[2][6] = {{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {1, -1}, {1, 1}},
                       {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}}};

bool isNearByPoint(const QPoint &a, const QPoint &b) {
    for(int i = 0; i < 6; i++) {
        if(a + nearby[a.y() % 2][i] == b) {
            return true;
        }
    }
    return false;
}

bool isNearByBlock(const BlockStatus *a, const BlockStatus *b) {
    return isNearByPoint(a->m_coord, b->m_coord);
}

// 只能在 camp 生成
bool canNewUnitAt(qint32 nowPlayer, const BlockStatus *a) {
    return ((nowPlayer == 1 && a->m_type == peopleCampBlock) ||
            (nowPlayer == 2 && a->m_type == virusCampBlock)) &&
        (a->m_unitOnBlock == -1);
}

// 看看一个 Unit 能否攻击周围的 Block，主要是有 Camp 或者 Unit
bool canUnitAttackBlock(const UnitStatus *a, const BlockStatus *b) {
    if(!isNearByPoint(a->m_nowCoord, b->m_coord)) {
        return false;
    }
    if((b->m_type & campBlock) == 0) {
        return false;
    }
    if(!notSameCamp(a, b)) {
        return false;
    }
    if(!a->isAlive() || b->getHP() < 0) {
        return false;
    }
    if(!a->canAttack()) {
        return false;
    }
    return true;
}

bool canUnitAttack(const UnitStatus *a, const UnitStatus *b) {
    // 没有检查 a 的主人是什么
    if(!isNearByPoint(a->m_nowCoord, b->m_nowCoord)) {
        return false;
    }
    if(!a->isAlive() || !b->isAlive()) {
        return false;
    }
    if(a->m_player == b->m_player) {
        return false;
    }
    if(!a->canAttack()) {
        return false;
    }
    return true;
}

// 获得所有周围的点的 Vector
QVector<QPoint> getNearbyPoint(const QPoint &a) {
    QVector<QPoint> ans;
    for(int i = 0; i < 6; i++) {
        ans.push_back(a + nearby[a.y() % 2][i]);
    }
    return ans;
}

// 获取在 scene 里面的 center
QPointF getBlockCenter(qint32 r, qint32 c) {
    // Q_ASSERT(1 <= r && r <= width());
    // Q_ASSERT(1 <= c && c <= height());
    return QPointF(qSqrt(3) * (r - 1) + (c % 2 == 0 ? qSqrt(3) / 2 : 0),
                   1.5 * (c - 1)) *
        GraphInfo::blockSize;
}

QPointF getBlockCenter(QPoint coord) {
    return getBlockCenter(coord.x(), coord.y());
}

bool notSameCamp(const UnitStatus *unit, const BlockStatus *block) {
    // uid 和 block 的 camp 相比
    return ((unit->m_type & peopleUnit) && block->m_type == virusCampBlock) ||
        ((unit->m_type & virusUnit) && block->m_type == peopleCampBlock);
}

BlockStatus::BlockStatus(const QJsonObject &json, QObject *parent) : QObject(parent) {
    this->read(json);
}
