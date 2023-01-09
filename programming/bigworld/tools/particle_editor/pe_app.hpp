#ifndef PE_APP_HPP
#define PE_APP_HPP

BW_BEGIN_NAMESPACE

class PeApp
{
public:
    PeApp();

    ~PeApp();

    static PeApp &instance();

private:
    // Not permitted
    PeApp(PeApp const &);
    PeApp &operator=(PeApp const &);

private:
    static PeApp        *s_instance;
};

BW_END_NAMESPACE

#endif // PE_APP_HPP
