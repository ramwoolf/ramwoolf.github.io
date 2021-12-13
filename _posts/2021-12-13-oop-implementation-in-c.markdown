---
layout: post
title:  "C++ ООП на Си"
date:   2021-12-06 15:46:00 +0100
categories: coding cpp
---
ООП в C++ все еще одна из ключевых парадигм. Принято выделять три классические характеристики ООП: инкапсуляция, наследование, полиморфизм. Шутки ради давайте реализуем эти базовые возможности на чистом Си.

Классический и заезженный пример - это иерархия shape <- rectangle. на мой взгляд, это слишком искусственно, непрактично и неинтересно. Более полезным может оказаться текстовый логгер. Не претендуя на идеальность реализации и пренебрегая всякими подробностями в виде разных кодировок и локалей, давайте реализуем его.

### Инкапсуляция

Под инкапсуляцией следует понимать локальность данных и операций над ними. В сиплюсплюсном классе мы определяем данные и операции для работы с ними, и неявно ипользуем указатель this, который передается "нулевым" параметром в функции-члены класса. На сях давайте определим тупую структуру и три тупые функции, которые будут принимать первым параметром указатель на тупую структуру.

{% highlight cpp %}
typedef struct 
{
    char *src_file;
} logger_t;

void logger_ctor(logger_t *self, char const* src);
void logger_dtor(logger_t *self);

void log_line(logger_t *self, char const* msg);
{% endhighlight %}

Отлично! Поместив эти определения в заголовочный файл, и написав какую-нибудь реализацию конструктора, деструктора и функции log_line (например, с выводом в stdout), мы можем подключить это к основному файлу и использовать.

{% highlight cpp %}
int main(int argc, char const *argv[])
{
    logger_t logger;
    logger_ctor(&logger, __FILE__);
    log_line(&logger, "Some message");
    log_line(&logger, "One more message");
    logger_dtor(&logger);
    return 0;
}
{% endhighlight %}

### Наследование

Производный класс в C++ содержит в себе базовый, конструктор производного сперва вызывает контруктор базового, а деструктор производного вызывает детруктор базового напоследок. 

Расширим пример с логгером -- перенесем логгер в stdout в производный класс и добавим логгер в текстовый файл , а корень иерархии сделаем "абстрактным классом". Вот так, например, будет выглядеть "класс" логгера в файл

{% highlight cpp %}
typedef struct {
    base_logger_t super;
    FILE* logfile;
    bool is_file_open;
} textfile_logger_t;
{% endhighlight %}

А его конструктор будет вызывать конструктор базового, передавая ему некоторые из параметров

{% highlight cpp %}
void textfile_logger_ctor(textfile_logger_t *self, char const* src) {
    base_logger_ctor(&self->super, src);
    ...
}
{% endhighlight %}

Таким образом получается публичное наследование, и члены базового класса доступны через поле super.

### Полиморфизм

Динамический полиморфизм и позднее связывание реализуются через vtbl -- таблицу указателей на функции, которые мы помечаем виртуальными, и vptr -- указатель на эту таблицу. При создании объекта производного класса, его виртуальные функции в процессе исполнения записываются в vtbl, через которую и происходит вся диспетчеризация. Добавим vtbl в базовый класс:

{% highlight cpp %}
struct LoggerVtbl;

typedef struct 
{
    struct logger_vtbl const * vptr;
    char *src_file;
} base_logger_t;

struct logger_vtbl {
    void (*log) (base_logger_t const * const self, time_t const* timestamp, char const* msg);
};
...
{% endhighlight %}

У нас будет одна виртуальная функция void log_that(base_logger_t const * loggers[], uint8_t n_loggers, char const* msg);, которую мы будем использовать вот так

{% highlight cpp %}
int main(int argc, char const *argv[])
{
    textfile_logger_t text_logger;
    terminal_logger_t terminal_logger;
    ...
    base_logger_t const* loggers[] = {
        &text_logger.super,
        &terminal_logger.super
    };

    log_that(loggers, sizeof(loggers)/sizeof(loggers[0]), "Some message");
    log_that(loggers, sizeof(loggers)/sizeof(loggers[0]), "One more message");
    ...
}
{% endhighlight %}

Во всех конструкторах, и в базовом, и в производных произведем инициализацию vptr

{% highlight cpp %}
void base_logger_ctor(base_logger_t *self, char const* src) {
    static struct logger_vtbl const vtbl = {
        &log_line_impl
    };

    self->vptr = &vtbl;
    ...
}
{% endhighlight %}

И затем добавим каждому классу реализацию log_line_impl с модификатором static. Реализация в базовом классе вызывает assert(0), имитируя ошибку при вызове метода у абстрактного класса, у остальных честно напишем логирование заданного сообщения с таймстампом и именем файла, откуда логирование производится.

Как сделать диспетчеризацию вызовов? У нас есть поле базового класса vptr, которому мы привоили при создании разные наборы реализаций виртуальных функций. В базовом класе сделаем вот такую прокси-функцию

{% highlight cpp %}
static inline void log_line(base_logger_t const * const logger, time_t const* timestamp, char const* msg) {
    (*logger->vptr->log)(logger, timestamp, msg);
}
{% endhighlight %}

Именно она и вызывает нужные реализации в зависимости от типа объекта.
Последний штрих -- связать "виртуальную" функцию с прокси-диспетчером

{% highlight cpp %}
void log_that(base_logger_t const * loggers[], uint8_t n_loggers, char const* msg) {
    time_t timestamp = time(NULL);

    uint8_t i = 0u;
    for (i = 0u; i < n_loggers; ++i) {
        log_line(loggers[i], &timestamp, msg);
    }
}
{% endhighlight %}

### Ремарки

Реализация получилась очень наивная. Всё публично, всё явно, руками вызываем все деструкторы, никакого специального синтаксиса. Но главое -- базовые идеи локальности данных и операций, иерархии классов и позднего связывания через vtbl реализованы и работают. Полный код можно найти вот [тут](https://github.com/ramwoolf/c_oop_logger)
